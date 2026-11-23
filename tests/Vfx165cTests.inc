// Round 165c (vfxfix165): the VFX leftovers of 0.1.19 — Level Scaler on held notes, Envelope loop points, Keyboard
// Splitter's zone-1 fallback and Manual mapping, Sequencer program changes and chains (core/Vfx.h)

namespace vfx165c
{
// the voice's velocity after each block of `n` samples (the last prop seen, else the note-on's)
std::vector<float> velocities (yw::vfx::Processor& p, int blocks, int n, yw::vfx::Transport t, std::vector<yw::vfx::Event> first)
{
    std::vector<float> out;
    float v = -1.0f;
    for (int b = 0; b < blocks; ++b)
    {
        yw::vfx::EventList o;
        if (b == 0)
            p.process (first.data(), (int) first.size(), n, t, o);
        else
            p.process (nullptr, 0, n, t, o);
        for (int k = 0; k < o.size(); ++k)
        {
            if (o[k].type == yw::vfx::Event::Type::on)
                v = o[k].voice.velocity;
            else if (o[k].type == yw::vfx::Event::Type::prop && o[k].prop == yw::vfx::Prop::velocity)
                v = o[k].value;
        }
        out.push_back (v);
    }
    return out;
}
} // namespace vfx165c

TEST (vfx165c_level_scaler_moves_held_notes)
{
    using namespace vfx164;
    using L = yw::vfx::LevelScalerSettings;
    auto p = yw::vfx::makeProcessor (yw::vfx::Kind::levelScaler, yw::vfx::writeLevelScaler (yw::vfx::defaultLevelScaler()));
    auto out = run (*p, { on (60, 0, 0, 0.9f) });
    CHECK (out.size() == 1 && std::abs (out[0].voice.velocity - 0.9f) < 1e-5f);
    const auto id = out.empty() ? 0 : out[0].id;
    CHECK (p->busy());
    // an automation of "Velocity - Multiplier" (0.5 → 0) and "Velocity - Offset" (0.75 → +4096): the held note follows
    p->setParameter (L::velocity * 6 + L::multiply, 0.5f);
    p->setParameter (L::velocity * 6 + L::offset, 0.75f);
    out = run (*p, {});
    CHECK (out.size() == 1 && out[0].type == yw::vfx::Event::Type::prop && out[0].prop == yw::vfx::Prop::velocity
           && out[0].id == id && std::abs (out[0].value - 0.5f) < 1e-4f);
    CHECK (run (*p, {}).empty()); // (nothing moved: nothing sent)
    // a VFX before it moves the voice's pan: the pan curve (identity) passes it on
    yw::vfx::Event pan;
    pan.type = yw::vfx::Event::Type::prop;
    pan.id = yw::vfx::inputId (60, 0);
    pan.prop = yw::vfx::Prop::pan;
    pan.value = -0.5f;
    out = run (*p, { pan });
    CHECK (out.size() == 1 && out[0].prop == yw::vfx::Prop::pan && std::abs (out[0].value + 0.5f) < 1e-5f);
    // the release goes out on the voice it sent
    out = run (*p, { off (60) });
    CHECK (out.size() == 1 && out[0].type == yw::vfx::Event::Type::off && out[0].id == id);
    CHECK (! p->busy());
    // a new state (the editor) also moves the held notes
    p = yw::vfx::makeProcessor (yw::vfx::Kind::levelScaler, yw::vfx::writeLevelScaler (yw::vfx::defaultLevelScaler()));
    run (*p, { on (62, 0, 0, 0.4f) });
    auto fixed = yw::vfx::defaultLevelScaler();
    fixed.at (L::velocity, L::multiply) = 0;
    fixed.at (L::velocity, L::offset) = 2048;
    CHECK (p->configure (yw::vfx::writeLevelScaler (fixed)));
    out = run (*p, {});
    CHECK (out.size() == 1 && out[0].prop == yw::vfx::Prop::velocity && std::abs (out[0].value - 0.25f) < 1e-4f);
}

TEST (vfx165c_level_scaler_shows_its_events)
{
    using namespace vfx164;
    using L = yw::vfx::LevelScalerSettings;
    auto s = yw::vfx::defaultLevelScaler();
    s.at (L::velocity, L::multiply) = 4096; // half the slope
    auto p = yw::vfx::makeProcessor (yw::vfx::Kind::levelScaler, yw::vfx::writeLevelScaler (s));
    yw::vfx::Monitor::Item items[16];
    run (*p, { on (60, 0, 0, 0.8f) });
    CHECK (p->monitor().pop (items, 16) == 0); // (no editor watching: nothing kept)
    p->monitor().watching = true;
    run (*p, { on (64, 0, 0, 0.8f) });
    const auto n = p->monitor().pop (items, 16);
    CHECK (n == 5); // pan, Mod X, Mod Y, fine pitch, velocity
    bool velocity = false;
    for (int k = 0; k < n; ++k)
        if (items[k].kind == L::velocity)
            velocity = std::abs (items[k].in - 0.8f) < 1e-5f && std::abs (items[k].out - 0.4f) < 1e-4f;
    CHECK (velocity);
    run (*p, { off (64) });
    CHECK (p->monitor().pop (items, 16) == 1 && items[0].kind == L::release);
}

TEST (vfx165c_envelope_loops_between_its_loop_and_sustain_points)
{
    using namespace vfx164;
    using E = yw::vfx::EnvelopeSettings;
    // FL's "Pumping": a tempo envelope (0,0) → (½ beat, 1) → (1 beat, 0 as a step), loop start 0, sustain 2
    auto s = yw::vfx::defaultEnvelope();
    auto& part = s.articulators[0].parts[E::velocity];
    part.envelope.points.clear();
    yw::vfx::MapCurve::Point a, b, c;
    b.x = 0.5;
    b.y = 1.0;
    c.x = 1.0;
    c.curve = 2;
    part.envelope.points = { a, b, c };
    part.envelope.footer = { 1, 1, 0, 0, 2 };
    yw::vfx::Transport t;
    t.sampleRate = 48000.0;
    t.bpm = 120.0; // 1 beat = 24000 samples
    auto p = yw::vfx::makeProcessor (yw::vfx::Kind::envelope, yw::vfx::writeEnvelope (s));
    const auto v = vfx165c::velocities (*p, 120, 600, t, { on (60, 0, 0, 0.78125f) }); // 3 beats in 1/40 beat blocks
    // a quarter beat in and a beat later: the same level (it loops); just before each beat the top, just after it the ramp from 0
    CHECK (std::abs (v[10] - v[50]) < 0.02f && std::abs (v[10] - v[90]) < 0.02f && v[10] > 0.2f);
    CHECK (v[35] > 0.7f && v[75] > 0.7f && v[115] > 0.7f);
    CHECK (v[41] < 0.3f && v[81] < 0.3f && v[41] < v[39] - 0.5f); // (the step back to 0, then the ramp again)
    // the release: from the sustain point on (its level 0 here), the loop ends
    yw::vfx::EventList o;
    auto offs = std::vector<yw::vfx::Event> { off (60) };
    p->process (offs.data(), 1, 600, t, o);
    bool released = false;
    for (int k = 0; k < 400 && p->busy(); ++k)
    {
        o.clear();
        p->process (nullptr, 0, 600, t, o);
        for (int e = 0; e < o.size(); ++e)
            released = released || o[e].type == yw::vfx::Event::Type::off;
    }
    CHECK (released || ! p->busy());
    // FL's own preset, when FL Studio is installed
    if (! realFlpDir.empty())
    {
        const auto file = std::filesystem::path (realFlpDir) / "Patches" / "Plugin presets" / "Effects" / "VFX Envelope" / "Pumping.fst";
        std::ifstream in (file, std::ios::binary);
        const std::vector<std::uint8_t> bytes ((std::istreambuf_iterator<char> (in)), {});
        if (const auto state = yw::vfx::stateFromFlPreset (bytes, yw::vfx::Kind::envelope))
        {
            p = yw::vfx::makeProcessor (yw::vfx::Kind::envelope, *state);
            const auto w = vfx165c::velocities (*p, 120, 600, t, { on (60, 0, 0, 0.78125f) });
            CHECK (std::abs (w[10] - w[50]) < 0.02f && std::abs (w[20] - w[100]) < 0.02f);
            CHECK (*std::max_element (w.begin() + 80, w.end()) > 0.5f); // (still pumping in the third beat)
        }
        else
            CHECK (false);
    }
}

TEST (vfx165c_keyboard_splitter_falls_back_on_zone_1)
{
    using namespace vfx164;
    using S = yw::vfx::KeyboardSplitterSettings;
    // zone 1: an ordinary zone for the low keys; zone 2: random, on the high keys only
    S s;
    s.zones[0].keyboard.points = { { 0.0, 1.0 }, { 59.0 / 132.0, 1.0 }, { 60.0 / 132.0, 0.0 } };
    s.zones[1].keyboard.points = { { 71.0 / 132.0, 0.0 }, { 72.0 / 132.0, 1.0 } };
    s.zones[1].random = true;
    auto p = yw::vfx::makeProcessor (yw::vfx::Kind::keyboardSplitter, yw::vfx::writeKeyboardSplitter (s));
    // key 80: zone 2 wins; key 64: no random zone has it, FL plays zone 1 (at full level, though its map is 0 there)
    auto out = run (*p, { on (80) });
    CHECK (out.size() == 1 && out[0].output == 1);
    out = run (*p, { on (64, 0, 0, 0.5f) });
    CHECK (out.size() == 1 && out[0].output == 0 && std::abs (out[0].voice.velocity - 0.5f) < 1e-4f);
    // zone 1 off: nothing
    s.zones[0] = {};
    p = yw::vfx::makeProcessor (yw::vfx::Kind::keyboardSplitter, yw::vfx::writeKeyboardSplitter (s));
    CHECK (run (*p, { on (64) }).empty());
}

TEST (vfx165c_keyboard_splitter_manual_mapping)
{
    using namespace vfx164;
    // FL's test: from Default, press 48, 60, 72 and release → zone 1 = 0…47, 2 = 48…59, 3 = 60…71, 4 = 72…131
    auto s = yw::vfx::defaultKeyboardSplitter();
    yw::vfx::SplitterMapping m;
    yw::vfx::beginManualMapping (m);
    CHECK (yw::vfx::manualMapKey (s, m, 48) && yw::vfx::manualMapKey (s, m, 60));
    CHECK (! yw::vfx::manualMapKey (s, m, 55)); // (keys must rise)
    CHECK (yw::vfx::manualMapKey (s, m, 72));
    yw::vfx::endManualMapping (s, m);
    CHECK (! m.on);
    auto zoneOf = [&] (int key) {
        int found = -1, count = 0;
        for (int z = 0; z < 16; ++z)
            if (s.zones[(std::size_t) z].keyboard.at (key / 132.0) > 0.999)
            {
                found = z;
                ++count;
            }
        return count == 1 ? found : -1;
    };
    CHECK (zoneOf (0) == 0 && zoneOf (47) == 0 && zoneOf (48) == 1 && zoneOf (59) == 1 && zoneOf (60) == 2 && zoneOf (71) == 2);
    CHECK (zoneOf (72) == 3 && zoneOf (131) == 3 && zoneOf (127) == 3);
    CHECK (! s.zones[4].active());
    // the processor plays the new map; while an editor maps, the notes are taken (and shown), not played
    auto p = yw::vfx::makeProcessor (yw::vfx::Kind::keyboardSplitter, yw::vfx::writeKeyboardSplitter (s));
    auto out = run (*p, { on (50), on (100) });
    CHECK (out.size() == 2 && out[0].output == 1 && out[1].output == 3);
    p->monitor().mapping = true;
    CHECK (run (*p, { on (40) }).empty());
    yw::vfx::Monitor::Item items[4];
    CHECK (p->monitor().pop (items, 4) == 1 && items[0].in == 40.0f);
    // fifteen keys: sixteen zones, the mapping ends by itself
    s = yw::vfx::defaultKeyboardSplitter();
    yw::vfx::beginManualMapping (m);
    for (int k = 0; k < 15; ++k)
        yw::vfx::manualMapKey (s, m, 40 + 3 * k);
    CHECK (! m.on && s.zones[15].active() && s.zones[15].keyboard.at (1.0) > 0.999 && s.zones[15].keyboard.at (80.0 / 132.0) < 1e-9);
}

namespace vfx165c
{
struct Heard
{
    double time = 0.0;
    int key = 0;
    bool on = true;
    bool program = false;
};

// 120 bpm / 48 kHz, the song playing from 0: the keys held from 0, `before (sample)` runs before each block (a program
// change, a new state…); returns what came out
std::vector<Heard> play (yw::vfx::Processor& p, std::vector<int> keys, double seconds, std::function<void (long long, std::vector<yw::vfx::Event>&)> before = {})
{
    constexpr int block = 480;
    constexpr double rate = 48000.0;
    std::vector<Heard> heard;
    yw::vfx::Transport t;
    t.sampleRate = rate;
    t.bpm = 120.0;
    t.playing = true;
    for (long long at = 0; at < (long long) (seconds * rate); at += block)
    {
        std::vector<yw::vfx::Event> in;
        if (at == 0)
            for (const auto k : keys)
                in.push_back (vfx164::on (k, 0, 0, 100.0f / 128.0f));
        if (before)
            before (at, in);
        t.ticks = (double) at / rate * 2.0 * 960.0;
        yw::vfx::EventList out;
        p.process (in.data(), (int) in.size(), block, t, out);
        out.sortByOffset();
        for (int k = 0; k < out.size(); ++k)
            if (out[k].type != yw::vfx::Event::Type::prop)
                heard.push_back ({ (double) (at + out[k].offset) / rate, out[k].voice.key, out[k].type == yw::vfx::Event::Type::on,
                                   out[k].type == yw::vfx::Event::Type::program });
    }
    return heard;
}

// the note-on keys in [from, to) seconds, their times shifted by `shift`
std::vector<std::pair<long long, int>> onsIn (const std::vector<Heard>& h, double from, double to, double shift = 0.0)
{
    std::vector<std::pair<long long, int>> v;
    for (const auto& e : h)
        if (e.on && ! e.program && e.time >= from - 1e-9 && e.time < to - 1e-9)
            v.emplace_back (std::llround ((e.time + shift) * 48000.0), e.key);
    return v;
}
} // namespace vfx165c

TEST (vfx165c_sequencer_program_change_selects_the_sequence)
{
    using namespace vfx164;
    using S = yw::vfx::SequencerSettings;
    auto s = yw::vfx::defaultSequencer();
    s.set (S::currentProgram, 2); // "Up 4 keys"
    auto up = vfx165c::play (*sequencer (s), { 60, 64, 67 }, 1.0);
    s.set (S::currentProgram, 3); // "Down 4 keys"
    auto down = vfx165c::play (*sequencer (s), { 60, 64, 67 }, 1.0);
    CHECK (vfx165c::onsIn (up, 0.0, 0.5) != vfx165c::onsIn (down, 0.0, 0.5));
    s.set (S::currentProgram, 2);
    auto changeAt = [] (long long sample) {
        return [sample] (long long at, std::vector<yw::vfx::Event>& in) {
            if (sample >= at && sample < at + 480)
                in.push_back (yw::vfx::programEvent (3, (int) (sample - at)));
        };
    };
    // "Program change" off (FL's default): nothing changes
    auto ignored = vfx165c::play (*sequencer (s), { 60, 64, 67 }, 1.0, changeAt (24000));
    CHECK (vfx165c::onsIn (ignored, 0.0, 1.0) == vfx165c::onsIn (up, 0.0, 1.0));
    // on: from the change (half a second in) the other sequence plays, on the same steps
    s.set (S::programChange, 1);
    auto changed = vfx165c::play (*sequencer (s), { 60, 64, 67 }, 1.0, changeAt (24000));
    CHECK (vfx165c::onsIn (changed, 0.0, 0.5) == vfx165c::onsIn (up, 0.0, 0.5));
    CHECK (vfx165c::onsIn (changed, 0.5, 1.0) == vfx165c::onsIn (down, 0.5, 1.0) && ! vfx165c::onsIn (down, 0.5, 1.0).empty());
    // the host's MIDI program change (Processor::programChange) does the same at the next block
    auto p = sequencer (s);
    auto viaHost = vfx165c::play (*p, { 60, 64, 67 }, 1.0, [&p] (long long at, std::vector<yw::vfx::Event>&) {
        if (at == 24000)
            p->programChange (3);
    });
    CHECK (vfx165c::onsIn (viaHost, 0.5, 1.0) == vfx165c::onsIn (down, 0.5, 1.0));
    // "pass on": it goes out for the instruments after it
    s.set (S::programChange, 2);
    auto passed = vfx165c::play (*sequencer (s), { 60, 64, 67 }, 1.0, changeAt (24000));
    CHECK (std::count_if (passed.begin(), passed.end(), [] (const vfx165c::Heard& h) { return h.program; }) == 1);
    CHECK (vfx165c::onsIn (passed, 0.0, 1.0) == vfx165c::onsIn (up, 0.0, 1.0));
}

TEST (vfx165c_sequencer_plays_its_chain)
{
    using namespace vfx164;
    using S = yw::vfx::SequencerSettings;
    auto s = yw::vfx::defaultSequencer();
    s.set (S::currentProgram, 2);
    auto up = vfx165c::play (*sequencer (s), { 60, 64, 67 }, 2.0);
    s.set (S::currentProgram, 3);
    auto down = vfx165c::play (*sequencer (s), { 60, 64, 67 }, 2.0);
    // chain 1 = "Up 4 keys", then "Down 4 keys" (each 16 sixteenths = 2 s at 120 bpm), then again
    s.chains[0].slots = { 2, -1, 3, -1, -1, -1, -1, -1 };
    s.set (S::currentChain, 0);
    s.set (S::currentProgram, 2);
    auto chained = vfx165c::play (*sequencer (s), { 60, 64, 67 }, 6.0);
    CHECK (vfx165c::onsIn (chained, 0.0, 2.0) == vfx165c::onsIn (up, 0.0, 2.0));
    CHECK (vfx165c::onsIn (chained, 2.0, 4.0, -2.0) == vfx165c::onsIn (down, 0.0, 2.0));
    CHECK (vfx165c::onsIn (chained, 4.0, 6.0, -4.0) == vfx165c::onsIn (up, 0.0, 2.0));
    // a program of 8 steps in the chain: it plays once (1 s), and the chain goes on
    auto shortProgram = s;
    shortProgram.programs[5] = s.programs[3];
    shortProgram.programs[5].params[(std::size_t) (S::steps - 39)] = 8;
    shortProgram.chains[0].slots = { 5, 2, -1, -1, -1, -1, -1, -1 };
    auto mixed = vfx165c::play (*sequencer (shortProgram), { 60, 64, 67 }, 3.0);
    CHECK (vfx165c::onsIn (mixed, 0.0, 1.0) == vfx165c::onsIn (down, 0.0, 1.0));
    CHECK (vfx165c::onsIn (mixed, 1.0, 3.0, -1.0) == vfx165c::onsIn (up, 0.0, 2.0));
    // round trip: the chain is kept in the state
    const auto again = yw::vfx::parseSequencer (yw::vfx::writeSequencer (s));
    CHECK (again && again->chains[0].slots[2] == 3 && again->get (S::currentChain) == 0);
}

TEST (vfx165c_sequencer_smart_bend_cuts_all_but_the_first)
{
    using namespace vfx164;
    using S = yw::vfx::SequencerSettings;
    auto s = blankProgram();
    for (int i = 0; i < 64; ++i)
        s.set (0x100 + i, S::chord);
    auto p = sequencer (s);
    auto bent = s;
    bent.set (S::smartBend, 1); // (no scale: a semitone)
    const auto fl = yw::vfx::writeSequencer (bent);
    auto heard = vfx165c::play (*p, { 60, 64, 67 }, 0.06, [&] (long long at, std::vector<yw::vfx::Event>&) {
        if (at == 960)
            p->configure (fl);
    });
    // block 0: the chord; block 2 (sample 960): two cut, the step plays again a semitone up
    std::vector<int> cut, again;
    for (const auto& h : heard)
        if (std::llround (h.time * 48000.0) == 960)
            (h.on ? again : cut).push_back (h.key);
    std::sort (cut.begin(), cut.end());
    std::sort (again.begin(), again.end());
    CHECK ((cut == std::vector<int> { 64, 67 })); // (the first, 60, keeps sounding: a mono instrument glides from it)
    CHECK ((again == std::vector<int> { 61, 65, 68 }));
}

namespace vfx165c
{
// the master's energy over `blocks` blocks of 512 at 48 kHz, the song playing
double songEnergy (const yw::Song& song, int blocks = 40)
{
    auto state = yw::RackState::build (song, nullptr, 48000.0);
    yw::RackRenderer r;
    r.prepare (48000.0, 512);
    std::vector<float> l (512), rr (512);
    double e = 0.0, tick = 0.0;
    for (int b = 0; b < blocks; ++b, tick += 512.0 * song.bpm * yw::ppq / 60.0 / 48000.0)
    {
        r.process (state.get(), true, tick, 512, l.data(), rr.data(), nullptr);
        for (const auto v : l)
            e += (double) v * v;
    }
    return e;
}

// channel 0 plays note 60 (colour `colour`) for a beat; channel 1 is a Patcher whose "MIDI port 6" pin plays a synth
yw::Song portSong (int colour = 0)
{
    using namespace patcher162;
    auto song = yw::Song::makeDefault();
    song.mixer.resize (4);
    song.channels.resize (2);
    song.channels[0] = {};
    song.channels[0].mixerTrack = 1;
    auto& target = song.channels[1];
    target = {};
    target.instrument = yw::InstrumentKind::patcher;
    target.mixerTrack = 1;
    target.patcher.nodes = { generatorNode (yw::InstrumentKind::synth) };
    target.patcher.wires = { midi (yw::patchFromHost, yw::patchMidiPortPin (6), 0, 0), patcher141::audio (0, yw::patchToHost) };
    yw::Pattern p;
    yw::Note n { 0, yw::ppq, 0, 60, 0.8f };
    n.colour = colour;
    p.notes.push_back (n);
    song.patterns = { p };
    yw::Clip clip;
    clip.pattern = 0;
    clip.length = yw::ppq * 4;
    song.playlist = { clip };
    return song;
}
} // namespace vfx165c

TEST (vfx165c_channel_rack_vfx_moves_its_voices_on_a_port)
{
    // a VFX Envelope channel on port 6 (FL's Default: the volume starts at 0, peaks at × 2 at 0.75 s): its voice
    // properties now reach the Patcher on that port, so the note swells instead of staying silent
    auto song = vfx165c::portSong();
    yw::vfx::makeChannel (song.channels[0], yw::vfx::Kind::envelope);
    yw::vfx::State st { yw::vfx::Kind::envelope, 6, yw::vfx::writeEnvelope (yw::vfx::defaultEnvelope()) };
    song.channels[0].pluginState = yw::vfx::writeState (st);
    const auto enveloped = vfx165c::songEnergy (song);
    auto plain = vfx165c::portSong (3); // (colour 3: no articulator, the note passes as it is)
    plain.channels[0] = song.channels[0];
    const auto passed = vfx165c::songEnergy (plain);
    std::printf ("  enveloped %.4g, passed %.4g\n", enveloped, passed);
    CHECK (passed > 1e-4 && enveloped > 0.05 * passed);
}

TEST (vfx165c_patcher_vfx_plays_on_to_yw_ports)
{
    using namespace patcher162;
    // channel 0: a Patcher, Notes → VFX Key Mapper (octave up) → "To YW" MIDI port 6; channel 1 takes port 6
    auto song = vfx165c::portSong();
    auto& source = song.channels[0];
    source.instrument = yw::InstrumentKind::patcher;
    yw::PatchNode v;
    v.generator = true;
    yw::vfx::makeChannel (v.channel, yw::vfx::Kind::keyMapper);
    yw::vfx::State st { yw::vfx::Kind::keyMapper, 0, vfx164::hex (vfx164::keyMapperOctaveUp) };
    v.channel.pluginState = yw::vfx::writeState (st);
    source.patcher.nodes = { v };
    source.patcher.wires = { midi (yw::patchFromHost, 0, 0, 0), midi (0, 0, yw::patchToHost, 6) };
    CHECK ((yw::patcher::vfxHostPorts (source.patcher) == std::vector<int> { 6 }));
    {
        auto state = yw::RackState::build (song, nullptr, 48000.0);
        const auto& routes = state->midiRoutesOut[0];
        CHECK (routes.size() == 1 && routes[0].other == 1 && routes[0].port == 6 && routes[0].portOnly);
        CHECK (state->portEventChannels == std::vector<int> { 0 });
    }
    CHECK (vfx165c::songEnergy (song) > 1e-4);
    // a Key Mapper that maps every key to nothing: silence, so the channel's own notes do not go out on that port
    auto mute = yw::vfx::defaultKeyMapper();
    for (auto& row : mute.rows)
        row.clear();
    auto silent = song;
    st.fl = yw::vfx::writeKeyMapper (mute);
    silent.channels[0].patcher.nodes[0].channel.pluginState = yw::vfx::writeState (st);
    CHECK (vfx165c::songEnergy (silent) == 0.0);
}
