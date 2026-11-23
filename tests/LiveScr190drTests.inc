// Round 190dr: the Session mixer's strip model (core/SessionMixer.h), the Device View's drag of a device to another
// track (core/DetailChain.h moveEffectToInsert) and the Clip page's Sample panel fields (core/ClipSample.h).
// docs/analise/REA_rodada190dr.md.

#include "core/ClipLauncher.h"
#include "core/ClipSample.h"
#include "core/DetailChain.h"
#include "core/SessionMixer.h"

namespace live190dr
{
inline yw::EffectSlot fx (yw::EffectKind kind, int row)
{
    yw::EffectSlot s;
    s.kind = kind;
    s.row = row;
    return s;
}
// Three tracks: "Bass" (a synth channel on insert 2, with an EQ and a compressor), "Vox" (an audio track linked to
// insert 3), "Empty" (nothing); insert 4 is a free bus. The compressor's mix is automated and MIDI-linked.
inline yw::Song song()
{
    yw::Song s;
    s.channels.resize (2);
    s.channels[0].name = "Bass synth";
    s.channels[0].instrument = yw::InstrumentKind::synth;
    s.channels[0].mixerTrack = 2;
    s.channels[1].name = "Vox take";
    s.channels[1].instrument = yw::InstrumentKind::audioClip;
    s.channels[1].mixerTrack = 3;
    if (s.mixer.size() < 6)
        s.mixer.resize (6);
    s.mixer[2].effects = { fx (yw::EffectKind::eq, 0), fx (yw::EffectKind::compressor, 2) };
    s.mixer[3].effects = { fx (yw::EffectKind::reverb, 0) };
    s.playlistTracks.resize (3);
    s.playlistTracks[0].name = "Bass";
    s.playlistTracks[1].name = "Vox";
    s.playlistTracks[1].mode = yw::PlaylistTrackMode::audio;
    s.playlistTracks[1].mixerTrack = 3;
    s.playlistTracks[2].name = "Empty";
    if (s.patterns.empty())
        s.patterns.emplace_back();
    yw::Note n;
    n.channel = 0;
    s.patterns[0].notes = { n };
    yw::Clip c;
    c.pattern = 0;
    c.length = 4 * yw::ppq;
    yw::launcher::setClip (s, 0, 0, c);
    yw::Automation a;
    a.target = { yw::AutomationTargetKind::effectMix, 2, 1, 0 };
    s.automations.push_back (a);
    yw::MidiLink link;
    link.target = { yw::AutomationTargetKind::effectParam, 2, 1, 3 };
    s.midiLinks.push_back (link);
    return s;
}
} // namespace live190dr

// --- Session mixer strips ----------------------------------------------------------------------------------------------

TEST (live190dr_strip_shows_the_insert_the_track_plays_through)
{
    using namespace yw::sessionmixer;
    const auto s = live190dr::song();
    const auto bass = stripOf (s, 0), vox = stripOf (s, 1), empty = stripOf (s, 2);
    CHECK (bass.insert == 2 && bass.insertCount == 1 && bass.notes);   // a MIDI track: "MIDI From"
    CHECK (vox.insert == 3 && ! vox.notes);                             // an audio track: "Audio From"
    CHECK (empty.insert == -1 && empty.insertCount == 0);               // nothing to show: the strip is empty
    CHECK (stripOf (s, 99).insert == -1);
}

TEST (live190dr_volume_box_reads_in_db_like_the_reference)
{
    using namespace yw::sessionmixer;
    CHECK (volumeText (yw::unityFader) == "0.00");
    CHECK (volumeText (0.0f) == "-inf");
    CHECK (volumeText (positionForDb (-3.0)) == "-3.00");
    CHECK (std::abs (volumeDb (positionForDb (-12.5)) - (-12.5)) < 0.01);
    CHECK (positionForDb (-INFINITY) == 0.0f);
    CHECK (std::abs (volumeDb (1.0f) - 5.6) < 0.1); // YW's fader top (+5.6 dB, FL's), Live's is +6 dB
}

TEST (live190dr_activator_and_exclusive_solo)
{
    using namespace yw::sessionmixer;
    auto s = live190dr::song();
    CHECK (isActive (s, 2));
    setActive (s, 2, false);
    CHECK (! isActive (s, 2) && s.mixer[2].muted);
    setActive (s, 2, true);
    CHECK (! s.mixer[2].muted);
    CHECK (! isActive (s, -1) && ! isActive (s, 0)); // no strip activator for "no insert" or the Master
    // exclusive (a plain click): only this one; again: none
    solo (s, 2, true);
    CHECK (s.mixer[2].solo && ! s.mixer[3].solo);
    solo (s, 3, true);
    CHECK (! s.mixer[2].solo && s.mixer[3].solo);
    solo (s, 2, false); // Ctrl adds
    CHECK (s.mixer[2].solo && s.mixer[3].solo);
    solo (s, 2, false); // and takes away
    CHECK (! s.mixer[2].solo && s.mixer[3].solo);
    solo (s, 3, true); // the only soloed track: clears
    CHECK (std::none_of (s.mixer.begin(), s.mixer.end(), [] (const yw::MixerTrack& m) { return m.solo; }));
}

TEST (live190dr_monitor_buttons_and_input_box)
{
    using namespace yw::sessionmixer;
    using M = yw::InputMonitor;
    CHECK (monitorOf (M::on) == Monitor::in && monitorOf (M::off) == Monitor::off);
    CHECK (monitorOf (M::automatic) == Monitor::autoMode && monitorOf (M::whenArmed) == Monitor::autoMode);
    CHECK (inputMonitorOf (Monitor::in) == M::on && inputMonitorOf (Monitor::autoMode) == M::automatic && inputMonitorOf (Monitor::off) == M::off);
    CHECK (inputChannelText (0, false) == "1" && inputChannelText (0, true) == "1/2" && inputChannelText (2, true) == "3/4");
    CHECK (inputChannelText (-1, true).empty());
}

TEST (live190dr_audio_to_main_or_another_insert)
{
    using namespace yw::sessionmixer;
    auto s = live190dr::song();
    yw::MixerSend toReverb;
    toReverb.target = 5;
    toReverb.level = 0.5f;
    s.mixer[2].sends = { toReverb };
    CHECK (outputOf (s, 2) == 0);
    CHECK (setOutput (s, 2, 4)); // Audio To: the bus
    CHECK (! s.mixer[2].toMaster && outputOf (s, 2) == 4 && s.mixer[2].sends.size() == 2);
    CHECK (s.mixer[2].sends.front().target == 4 && s.mixer[2].sends.front().level == 1.0f);
    CHECK (s.mixer[2].sends[1].target == 5 && s.mixer[2].sends[1].level == 0.5f); // the send stays
    CHECK (! setOutput (s, 2, 4));             // already there
    CHECK (! setOutput (s, 4, 2));             // 4 -> 2 would be a loop (2 goes to 4)
    CHECK (! setOutput (s, 2, 2));
    const auto choices = outputChoices (s, 4);
    CHECK (choices.front() == 0 && std::find (choices.begin(), choices.end(), 2) == choices.end());
    CHECK (std::find (choices.begin(), choices.end(), 3) != choices.end());
    CHECK (setOutput (s, 2, 0)); // back to Main: the route that was the output goes, the send stays
    CHECK (s.mixer[2].toMaster && s.mixer[2].sends.size() == 1 && s.mixer[2].sends.front().target == 5);
    s.mixer[3].toMaster = false;
    s.mixer[3].sends.clear();
    CHECK (outputOf (s, 3) == -1); // sends only
}

TEST (live190dr_send_rows_and_knobs)
{
    using namespace yw::sessionmixer;
    auto s = live190dr::song();
    CHECK (sendRows (s, { 2, 3 }, 8).empty());
    CHECK (sendPosition (s, 2, 5) < 0.0f);
    CHECK (! setSendPosition (s, 2, 5, 0.0f)); // a knob at zero makes no send
    CHECK (setSendPosition (s, 2, 5, yw::unityFader)); // turned up: a send at 0 dB
    CHECK (s.mixer[2].sends.size() == 1 && std::abs (s.mixer[2].sends[0].level - 1.0f) < 1.0e-4f);
    CHECK (std::abs (sendPosition (s, 2, 5) - yw::unityFader) < 1.0e-4f);
    CHECK (setSendPosition (s, 3, 4, 0.5f));
    CHECK ((sendRows (s, { 2, 3 }, 8) == std::vector<int> { 4, 5 }));
    CHECK ((sendRows (s, { 2, 3 }, 1) == std::vector<int> { 4 }));
    CHECK (setSendPosition (s, 2, 5, 0.0f)); // down to zero: the send stays, silent (FL keeps a route at 0)
    CHECK (s.mixer[2].sends.size() == 1 && s.mixer[2].sends[0].level == 0.0f);
    CHECK (! setSendPosition (s, 5, 2, 0.6f)); // 2 sends to 5: 5 -> 2 would loop
    CHECK (! setSendPosition (s, 2, 2, 0.6f));
}

TEST (live190dr_track_delay_is_the_manual_latency_with_the_reference_sign)
{
    using namespace yw::sessionmixer;
    auto s = live190dr::song();
    setTrackDelayMs (s, 2, 12.5); // Live: +12.5 ms = the track is later
    CHECK (s.mixer[2].latencyMs == -12.5f && trackDelayMs (s, 2) == 12.5);
    setTrackDelayMs (s, 2, -99999.0);
    CHECK (trackDelayMs (s, 2) == -(double) yw::maxTrackLatencyMs); // (clamped to YW's range)
    CHECK (trackDelayMs (s, -1) == 0.0);
}

TEST (live190dr_sections_follow_the_view_and_fit_the_room)
{
    using namespace yw::sessionmixer;
    yw::SessionMixerView view; // Live's default set: In/Out, Sends, Volume on; Track Options off
    CHECK (view.shown && view.io && view.sends && view.volume && ! view.trackOptions);
    auto h = sectionHeights (view, 2, 1000);
    CHECK (h.io == ioHeight && h.sends == 2 * sendRowHeight + 6 && h.trackOptions == 0 && h.volume == volumeHeight);
    CHECK (sectionHeights (view, 0, 1000).sends == 0); // no send anywhere: no Sends rows
    h = sectionHeights (view, 2, volumeHeight + 10); // little room: In/Out and Sends give way first
    CHECK (h.io == 0 && h.sends == 0 && h.volume == volumeHeight);
    CHECK (sectionHeights (view, 2, 20).total() == 0);
    view.shown = false;
    CHECK (sectionHeights (view, 2, 1000).total() == 0);
    yw::ClipGrid grid;
    CHECK (grid.mixerView == yw::SessionMixerView {}); // projects saved before this round: the defaults
}

// --- Device View: a device dragged to another track ---------------------------------------------------------------

TEST (live190dr_device_moves_to_another_track_with_its_automation)
{
    auto s = live190dr::song();
    const auto instance = s.mixer[2].effects[1].instance.id;
    const auto at = yw::detailchain::moveEffectToInsert (s, 2, 1, 3, false);
    CHECK (at == 1); // at the end of the Vox chain, after its reverb
    CHECK (s.mixer[2].effects.size() == 1 && s.mixer[2].effects[0].kind == yw::EffectKind::eq);
    CHECK (s.mixer[3].effects.size() == 2 && s.mixer[3].effects[1].kind == yw::EffectKind::compressor);
    CHECK (s.mixer[3].effects[1].instance.id == instance); // the same device, not a copy
    CHECK (s.automations[0].target.kind == yw::AutomationTargetKind::effectMix && s.automations[0].target.index == 3 && s.automations[0].target.slot == 1);
    CHECK (s.midiLinks[0].target.index == 3 && s.midiLinks[0].target.slot == 1 && s.midiLinks[0].target.param == 3);
    // and back: the chains are as they were (the move is its own inverse)
    const auto before = live190dr::song();
    CHECK (yw::detailchain::moveEffectToInsert (s, 3, 1, 2, false) == 1);
    CHECK (s.mixer[2].effects.size() == 2 && s.mixer[2].effects[1].kind == yw::EffectKind::compressor && s.mixer[2].effects[1].instance.id == instance);
    CHECK (s.automations[0].target == before.automations[0].target && s.midiLinks[0].target == before.midiLinks[0].target);
    CHECK (s.mixer[3].effects.size() == 1);
}

TEST (live190dr_ctrl_drag_copies_to_another_track)
{
    auto s = live190dr::song();
    const auto at = yw::detailchain::moveEffectToInsert (s, 2, 1, 3, true);
    CHECK (at == 1 && s.mixer[2].effects.size() == 2 && s.mixer[3].effects.size() == 2);
    CHECK (s.mixer[3].effects[1].kind == yw::EffectKind::compressor);
    CHECK (s.mixer[3].effects[1].instance.id != s.mixer[2].effects[1].instance.id); // a new device
    CHECK (s.automations[0].target.index == 2 && s.automations[0].target.slot == 1); // the automation stays with the original
}

TEST (live190dr_device_drag_refusals_and_side_inputs)
{
    auto s = live190dr::song();
    const auto untouched = s;
    CHECK (yw::detailchain::moveEffectToInsert (s, 2, 1, 2, false) == -1); // the same track: the chain drag does it
    CHECK (yw::detailchain::moveEffectToInsert (s, 2, 5, 3, false) == -1);
    CHECK (yw::detailchain::moveEffectToInsert (s, 2, 0, 99, false) == -1);
    s.mixer[4].effects.clear();
    for (int i = 0; i < yw::maxEffectSlots; ++i)
        s.mixer[4].effects.push_back (live190dr::fx (yw::EffectKind::eq, i));
    CHECK (yw::detailchain::moveEffectToInsert (s, 2, 1, 4, false) == -1); // full: nothing moves
    CHECK (s.mixer[2].effects == untouched.mixer[2].effects);
    // a compressor listening to Vox, moved onto Vox: it can't listen to itself
    s = live190dr::song();
    s.mixer[2].effects[1].sidechain = 3;
    CHECK (yw::detailchain::moveEffectToInsert (s, 2, 1, 3, false) == 1 && s.mixer[3].effects[1].sidechain == -1);
}

// --- Clip page: the Sample panel ------------------------------------------------------------------------------------

TEST (live190dr_warp_switch_follows_the_tempo_and_keeps_the_region)
{
    using namespace yw::clipsample;
    yw::Song s;
    s.bpm = 120.0;
    s.channels.resize (1);
    s.channels[0].instrument = yw::InstrumentKind::audioClip;
    auto& sampler = s.channels[0].sampler;
    sampler.stretchBeats = 16.0f; // a 4 s file warped to 16 beats (at 120 BPM it would last 8)
    sampler.keepPitch = true;
    yw::Clip clip;
    clip.kind = yw::ClipKind::audio;
    clip.channel = 0;
    clip.offset = 4 * yw::ppq; // beats 4..12 of the warped content = seconds 1..3 of the file
    clip.length = 8 * yw::ppq;
    yw::launcher::setClip (s, 0, 0, clip);
    CHECK (isWarped (s.channels[0].sampler));
    CHECK (setWarp (s, 0, false, 4.0));
    CHECK (! isWarped (s.channels[0].sampler));
    const auto* c = yw::launcher::clipAt (*s.clipGrid, 0, 0);
    CHECK (c != nullptr && c->offset == 2 * yw::ppq && c->length == 4 * yw::ppq); // still seconds 1..3, now at its own speed
    CHECK (! setWarp (s, 0, false, 4.0));
    // on again, no markers: the file's length at the song tempo (no change now, it follows the tempo from here)
    CHECK (setWarp (s, 0, true, 4.0));
    CHECK (s.channels[0].sampler.stretchBeats == 8.0f && c->offset == 2 * yw::ppq && c->length == 4 * yw::ppq);
    // with warp markers: they stay while Warp is off and give the length back when it is on
    auto& w = s.channels[0].sampler;
    w.warp = { { 0.0, 0.0 }, { 2.0, 6.0 }, { 4.0, 10.0 } };
    yw::warpedit::syncLength (w, 4.0);
    CHECK (w.stretchBeats == 10.0f);
    CHECK (setWarp (s, 0, false, 4.0) && w.warp.size() == 3 && w.stretchBeats == 0.0f);
    CHECK (setWarp (s, 0, true, 4.0) && w.stretchBeats == 10.0f);
    CHECK (! setWarp (s, 5, true, 4.0) && ! setWarp (s, 0, false, 0.0));
}

TEST (live190dr_set_start_and_end_at_the_play_position)
{
    using namespace yw::clipsample;
    yw::Clip c;
    c.offset = yw::ppq;
    c.length = 8 * yw::ppq;
    c.fadeIn = 6 * yw::ppq;
    CHECK (playPosition (c, 0.0f) == yw::ppq && playPosition (c, 0.5f) == 5 * yw::ppq && playPosition (c, 2.0f) == 9 * yw::ppq);
    auto start = c;
    CHECK (setStartAt (start, 5 * yw::ppq));
    CHECK (start.offset == 5 * yw::ppq && start.offset + start.length == 9 * yw::ppq && start.fadeIn == 4 * yw::ppq);
    CHECK (! setStartAt (start, 9 * yw::ppq) && ! setStartAt (start, -1));
    auto end = c;
    CHECK (setEndAt (end, 3 * yw::ppq));
    CHECK (end.offset == yw::ppq && end.length == 2 * yw::ppq && end.fadeIn == 2 * yw::ppq);
    CHECK (! setEndAt (end, yw::ppq));
}

TEST (live190dr_sample_title_reads_like_the_reference)
{
    using namespace yw::clipsample;
    CHECK (infoText ("loop16.wav", 48000.0, 16, 2) == "loop16.wav  48.0 kHz \xc2\xb7 16-Bit \xc2\xb7 2 Ch");
    CHECK (infoText ("take.flac", 44100.0, 24, 1) == "take.flac  44.1 kHz \xc2\xb7 24-Bit \xc2\xb7 1 Ch");
    CHECK (infoText ("x.mp3", 44100.0, 0, 2) == "x.mp3  44.1 kHz \xc2\xb7 2 Ch");
    CHECK (infoText ("missing.wav", 0.0, 0, 0) == "missing.wav");
}

// --- round 190dr-2: the Clip section's fields, the BPM box and Fade -------------------------------------------------

TEST (live190dr2_fields_read_bars_beats_sixteenths_like_the_reference)
{
    using namespace yw::clipsample;
    const auto bar = 4 * yw::ppq;
    CHECK (positionText (0, yw::Meter (4, 4)) == "1. 1. 1");                       // Start 1. 1. 1
    CHECK (positionText (8 * bar, yw::Meter (4, 4)) == "9. 1. 1");                 // End 9. 1. 1
    CHECK (positionText (8 * bar, yw::Meter (4, 4), true) == "8. 0. 0");           // Length 8. 0. 0
    CHECK (positionText (bar + 2 * yw::ppq + yw::ppq / 4 * 3, yw::Meter (4, 4)) == "2. 3. 4");
    CHECK (positionText (7 * yw::ppq, yw::Meter (3, 4)) == "3. 2. 1");           // 3/4: a bar is three beats
    CHECK (parsePosition ("9. 1. 1", yw::Meter (4, 4)) == 8 * bar && parsePosition ("9.1.1", yw::Meter (4, 4)) == 8 * bar && parsePosition ("9", yw::Meter (4, 4)) == 8 * bar);
    CHECK (parsePosition ("2. 3. 4", yw::Meter (4, 4)) == bar + 2 * yw::ppq + yw::ppq / 4 * 3);
    CHECK (parsePosition ("8. 0. 0", yw::Meter (4, 4), true) == 8 * bar && parsePosition ("0. 2", yw::Meter (4, 4), true) == 2 * yw::ppq);
    CHECK (parsePosition ("0. 1. 1", yw::Meter (4, 4)) == -1 && parsePosition ("abc", yw::Meter (4, 4)) == -1 && parsePosition ("", yw::Meter (4, 4)) == -1 && parsePosition ("-1", yw::Meter (4, 4), true) == -1);
    // unwarped audio: seconds at the song tempo (120 BPM: one beat = 0.5 s)
    CHECK (secondsText (0, 120.0) == "0:00:000" && secondsText (3 * yw::ppq, 120.0) == "0:01:500" && secondsText (250 * yw::ppq, 120.0) == "2:05:000");
    CHECK (parseSeconds ("0:01:500", 120.0) == 3 * yw::ppq && parseSeconds ("1.5", 120.0) == 3 * yw::ppq && parseSeconds ("01:500", 120.0) == 3 * yw::ppq);
    CHECK (parseSeconds ("x", 120.0) == -1);
}

TEST (live190dr2_position_moves_the_loop_keeping_its_length)
{
    using namespace yw::clipsample;
    yw::Clip c;
    c.offset = yw::ppq;
    c.length = 8 * yw::ppq;
    CHECK (setPositionAt (c, 4 * yw::ppq) && c.offset == 4 * yw::ppq && c.length == 8 * yw::ppq);
    CHECK (! setPositionAt (c, -1));
}

TEST (live190dr2_bpm_box_and_halve_double_scale_the_clip)
{
    using namespace yw::clipsample;
    yw::Song s;
    s.bpm = 120.0;
    s.channels.resize (1);
    s.channels[0].instrument = yw::InstrumentKind::audioClip;
    s.channels[0].sampler.stretchBeats = 8.0f; // a 4 s file read at 120 BPM
    yw::Clip clip;
    clip.kind = yw::ClipKind::audio;
    clip.channel = 0;
    clip.length = 8 * yw::ppq;
    yw::launcher::setClip (s, 0, 0, clip);
    CHECK (std::abs (fileBpm (s.channels[0].sampler, 4.0) - 120.0) < 1.0e-9);
    // ÷2 (Live: 120 -> 60 BPM, the 2-bar clip becomes one bar)
    CHECK (scaleWarpedTempo (s, 0, 0, 0, 4.0, 0.5));
    CHECK (std::abs (fileBpm (s.channels[0].sampler, 4.0) - 60.0) < 1.0e-9 && yw::launcher::clipAt (*s.clipGrid, 0, 0)->length == 4 * yw::ppq);
    CHECK (setFileBpm (s, 0, 0, 0, 4.0, 120.0)); // the box typed back to 120
    CHECK (s.channels[0].sampler.stretchBeats == 8.0f && yw::launcher::clipAt (*s.clipGrid, 0, 0)->length == 8 * yw::ppq);
    // with warp markers: they scale, the length follows them
    s.channels[0].sampler.warp = { { 0.0, 0.0 }, { 2.0, 3.0 }, { 4.0, 8.0 } };
    CHECK (scaleWarpedTempo (s, 0, 0, 0, 4.0, 2.0) && s.channels[0].sampler.warp[1].beats == 6.0 && s.channels[0].sampler.stretchBeats == 16.0f);
    s.channels[0].sampler.stretchBeats = 0.0f; // unwarped: no BPM, no ÷2
    CHECK (fileBpm (s.channels[0].sampler, 4.0) == 0.0 && ! scaleWarpedTempo (s, 0, 0, 0, 4.0, 0.5));
}

TEST (live190dr2_fade_switch_sets_short_edge_fades)
{
    using namespace yw::clipsample;
    yw::Clip c;
    c.length = 8 * yw::ppq;
    CHECK (! hasEdgeFades (c));
    setEdgeFades (c, true, 120.0);
    const auto expect = (std::int64_t) std::llround (edgeFadeMs / 1000.0 * 2.0 * yw::ppq);
    CHECK (hasEdgeFades (c) && c.fadeIn == std::max<std::int64_t> (1, expect) && c.fadeOut == c.fadeIn);
    setEdgeFades (c, false, 120.0);
    CHECK (! hasEdgeFades (c));
}
