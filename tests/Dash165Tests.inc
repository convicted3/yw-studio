// Round 165c dash165 (docs/analise/REA_rodada165c-dash165.md): FL's curve point law (core/FlCurve.h), the X-Y-Z
// Controller's curve modes, the Dashboard's MIDI and its 1024 outputs, the Voltage Controller's own parameters as
// automation / link targets.

#include "core/FlCurve.h"
#include "core/FruityShapers.h"

namespace dash165
{
// a Dashboard channel with the given controls and MIDI settings
yw::ChannelDef dashboard (std::vector<yw::SurfaceControl> controls, const yw::surface::DashboardMidi& midi)
{
    yw::ChannelDef c;
    c.instrument = yw::InstrumentKind::dashboard;
    c.patcher.surface = std::move (controls);
    yw::surface::writeDashboardMidi (c.pluginState, midi);
    return c;
}

yw::SurfaceControl control (yw::SurfaceControl::Midi midi, int number, float value, int low = 0, int high = 127)
{
    yw::SurfaceControl c;
    c.kind = yw::SurfaceControl::Kind::knob;
    c.name = "Knob";
    c.value = value;
    c.midi = midi;
    c.midiNumber = number;
    c.midiLow = low;
    c.midiHigh = high;
    return c;
}

std::vector<std::vector<std::uint8_t>> take (yw::Instrument& i)
{
    std::array<std::uint32_t, 192> m {};
    std::vector<std::vector<std::uint8_t>> out;
    for (int round = 0; round < 64; ++round)
    {
        const auto n = i.takeMidiMessages (m.data(), (int) m.size());
        if (n <= 0)
            break;
        for (int k = 0; k < n; ++k)
        {
            const auto status = (std::uint8_t) (m[(std::size_t) k] & 0xff);
            std::vector<std::uint8_t> b { status, (std::uint8_t) ((m[(std::size_t) k] >> 8) & 127) };
            if ((status & 0xE0) != 0xC0)
                b.push_back ((std::uint8_t) ((m[(std::size_t) k] >> 16) & 127));
            out.push_back (b);
        }
    }
    return out;
}
} // namespace dash165

TEST (flcurve165_ends_straight_and_counts)
{
    using namespace yw::flcurve;
    // zero tension: straight in the curve modes (FL's mask 0x1B83)
    for (const auto m : { singleCurve, doubleCurve, singleCurve2, doubleCurve2, halfSine, singleCurve3, doubleCurve3 })
        for (const auto x : { 0.0, 0.2, 0.5, 0.9, 1.0 })
            CHECK (std::abs (shape (m, 0.0f, x) - x) < 1e-12);
    // the curve modes keep their ends and bend either way
    for (const auto m : { singleCurve, doubleCurve, singleCurve2, doubleCurve2, halfSine, singleCurve3, doubleCurve3 })
        for (const auto t : { -1.0f, -0.725f, -0.28f, 0.28f, 0.65f, 1.0f })
        {
            CHECK (std::abs (shape (m, t, 0.0)) < 1e-5 && std::abs (shape (m, t, 1.0) - 1.0) < 1e-5);
            double last = -1.0;
            bool monotonic = true;
            for (int i = 0; i <= 200; ++i)
            {
                const auto y = shape (m, t, i / 200.0);
                monotonic = monotonic && y >= last - 1e-9;
                last = y;
            }
            CHECK (monotonic);
        }
    CHECK (shape (singleCurve, 0.5f, 0.5) > 0.5 && shape (singleCurve, -0.5f, 0.5) < 0.5);
    CHECK (shape (hold, 0.3f, 0.7) == 0.0);
    // FL's step count: round (0.001 (1000001^(1 − 0.6 |t|) − 1)), stairs + 1 when t ≥ 0
    CHECK (count (stairs, 0.0f) == 1001 && count (smoothStairs, 0.0f) == 1000 && count (pulse, 1.0f) == 0 && count (stairs, 1.0f) == 1);
    CHECK (count (wave, -1.0f) == 0 && count (singleCurve, 0.5f) == -1);
    // stairs at full tension: two levels (0 then 1), pulse alternates
    CHECK (shape (stairs, 1.0f, 0.2) == 0.0 && shape (stairs, 1.0f, 0.8) == 1.0);
    CHECK (std::string (modeName (stairs)) == "Stairs" && std::string (modeName (99)) == "Double curve 3");
}

TEST (flcurve165_matches_the_waveshaper_law)
{
    // FL's WaveShaper map (core/FruityShapers, null-tested against FL in REA_rodada51) evaluates the same point law: a
    // 2-point map 0 → 1 with point 2's mode / tension is the curve itself (its table holds round (y × 32767))
    using namespace yw::flcurve;
    int compared = 0, worst = 0;
    for (int m = 0; m < modeCount; ++m)
    {
        if (m == smooth || m == hold)
            continue;
        for (const auto t : { -1.0f, -0.725f, -0.4f, -0.28f, 0.0f, 0.28f, 0.5f, 0.65f, 1.0f })
        {
            const std::vector<yw::flshape::WsPoint> pts { { 0.0f, 0.0f, 0.0f, 0 }, { 1.0f, 1.0f, t, (std::uint8_t) m } };
            const auto table = yw::flshape::shaperTable (pts, false);
            const int n = (int) table.size() - 3; // N
            for (int i = 1; i < n; i += 97)
            {
                const auto want = std::nearbyint (shape (m, t, (double) i / n) * 32767.0);
                // (the steps of modes 3…5 may land one table entry apart at an edge)
                const auto got = (double) table[(std::size_t) i + 1];
                const auto d = (int) std::abs (got - want);
                if (m >= stairs && m <= pulse && d > 2)
                {
                    const auto a = std::nearbyint (shape (m, t, (double) (i - 1) / n) * 32767.0);
                    const auto b = std::nearbyint (shape (m, t, (double) (i + 1) / n) * 32767.0);
                    CHECK (std::abs (got - a) <= 2 || std::abs (got - b) <= 2);
                }
                else
                {
                    worst = std::max (worst, d);
                    CHECK (d <= 2);
                }
                ++compared;
            }
        }
    }
    std::printf ("  FL curve vs WaveShaper law: %d points, worst %d / 32767\n", compared, worst);
}

TEST (xyz165_curve_modes)
{
    using namespace yw;
    const auto kind = EffectKind::xyzController;
    CHECK (xy::paramCount (kind) == 17 && (int) effectInfo (kind).params.size() == 17);
    CHECK (effectInfo (kind).params[(std::size_t) xy::curveMode (1)].name == std::string ("Y curve"));
    CHECK (xy::paramCount (EffectKind::xyController) == 10);
    auto p = xy::defaults (kind);
    // default: single curve, no tension = straight
    CHECK (xy::curveModeOf (p[(std::size_t) xy::curveMode (0)]) == flcurve::singleCurve);
    CHECK (std::abs (xy::output (kind, p, 0, 0.3) - 0.3f) < 1e-6f);
    // stairs with full tension on Z: 0 below the middle, 1 above; min / max still scale it
    p[(std::size_t) xy::curveMode (2)] = xy::curveModeValue (flcurve::stairs);
    p[(std::size_t) xy::tension (2)] = 1.0f;
    p[(std::size_t) xy::maximum (2)] = 0.8f;
    CHECK (xy::output (kind, p, 2, 0.2) == 0.0f && std::abs (xy::output (kind, p, 2, 0.8) - 0.8f) < 1e-6f);
    // a tension bends the X-Y-Z with FL's point law (−1…1), not the X-Y Controller's
    p[(std::size_t) xy::tension (0)] = 0.75f; // FL's +0.5
    CHECK (std::abs (xy::output (kind, p, 0, 0.5) - (float) flcurve::shape (flcurve::singleCurve, 0.5f, 0.5)) < 1e-6f);
    CHECK (xy::paramText (kind, xy::curveMode (2), p[(std::size_t) xy::curveMode (2)]) == "Stairs");
    CHECK (xy::paramText (kind, xy::tension (0), 0.75f) == "+50%");
    for (int m = 0; m < flcurve::modeCount; ++m)
        CHECK (xy::curveModeOf (xy::curveModeValue (m)) == m);
}

TEST (xyz165_fl_state_curve_tension_and_mode)
{
    // FL's X-Y-Z state (version 3): its output curves' second point carries the mode and tension YW now maps
    std::vector<std::uint8_t> d (56 + 3 * (8 + 160), 0);
    const std::int32_t version = 3;
    std::memcpy (d.data(), &version, 4);
    for (int a = 0; a < 3; ++a)
    {
        const auto curve = (std::size_t) 56 + (std::size_t) a * (8 + 160) + 8 + 80;
        const std::int32_t points = 2;
        std::memcpy (d.data() + curve + 8, &points, 4);
        const double x0 = 0, y0 = 0.1, x1 = 1, y1 = 0.9;
        const float t = a == 0 ? 0.65f : a == 1 ? -0.28f : 0.0f;
        std::memcpy (d.data() + curve + 12, &x0, 8);
        std::memcpy (d.data() + curve + 20, &y0, 8);
        std::memcpy (d.data() + curve + 36, &x1, 8);
        std::memcpy (d.data() + curve + 44, &y1, 8);
        std::memcpy (d.data() + curve + 52, &t, 4);
        d[curve + 56] = (std::uint8_t) (a == 2 ? yw::flcurve::wave : yw::flcurve::singleCurve);
    }
    const auto slot = yw::xy::fromFl ("Fruity X-Y-Z Controller", d);
    CHECK (slot.has_value());
    if (! slot)
        return;
    CHECK (slot->kind == yw::EffectKind::xyzController);
    CHECK (std::abs (slot->params[(std::size_t) yw::xy::tension (0)] - (0.65f * 0.5f + 0.5f)) < 1e-6f);
    CHECK (std::abs (slot->params[(std::size_t) yw::xy::tension (1)] - (-0.28f * 0.5f + 0.5f)) < 1e-6f);
    CHECK (yw::xy::curveModeOf (slot->params[(std::size_t) yw::xy::curveMode (2)]) == yw::flcurve::wave);
    CHECK (std::abs (slot->params[(std::size_t) yw::xy::minimum (0)] - 0.1f) < 1e-6f && std::abs (slot->params[(std::size_t) yw::xy::maximum (0)] - 0.9f) < 1e-6f);
    // the output at position 0.5 follows FL's curve between the ends
    const auto want = 0.1 + 0.8 * yw::flcurve::shape (yw::flcurve::singleCurve, 0.65f, 0.5);
    CHECK (std::abs (yw::xy::output (slot->kind, slot->params, 0, 0.5) - (float) want) < 1e-5f);
}

TEST (dashboard165_midi_messages)
{
    using M = yw::SurfaceControl::Midi;
    yw::surface::MidiBytes m[6];
    CHECK (yw::surface::midiOf (dash165::control (M::cc, 74, 0.5f), 0.5f, 2, m) == 1 && m[0].bytes[0] == 0xB2 && m[0].bytes[1] == 74
           && m[0].bytes[2] == 64 && m[0].size == 3);
    CHECK (yw::surface::midiOf (dash165::control (M::none, 74, 0.5f), 0.5f, 0, m) == 0);
    // 14-bit CC: high 7 bits, then the low ones on the same CC
    CHECK (yw::surface::midiOf (dash165::control (M::cc, 1, 1.0f, 0, 16383), 1.0f, 0, m) == 2 && m[0].bytes[2] == 127 && m[1].bytes[2] == 127
           && m[0].bytes[1] == 1 && m[1].bytes[1] == 1);
    // NRPN 300: CC 99 / 98 select, CC 6 data
    CHECK (yw::surface::midiOf (dash165::control (M::nrpn, 300, 1.0f), 1.0f, 0, m) == 3 && m[0].bytes[1] == 99 && m[0].bytes[2] == 2
           && m[1].bytes[1] == 98 && m[1].bytes[2] == 44 && m[2].bytes[1] == 6 && m[2].bytes[2] == 127);
    CHECK (yw::surface::midiOf (dash165::control (M::rpn, 0, 0.0f, 0, 16383), 0.0f, 0, m) == 4 && m[0].bytes[1] == 101 && m[3].bytes[1] == 38);
    CHECK (yw::surface::midiOf (dash165::control (M::pressure, 0, 0.5f), 0.5f, 15, m) == 1 && m[0].bytes[0] == 0xDF && m[0].size == 2);
    CHECK (yw::surface::midiOf (dash165::control (M::note, 60, 1.0f), 1.0f, 0, m) == 1 && m[0].bytes[0] == 0x90 && m[0].bytes[2] == 127);
    CHECK (yw::surface::midiOf (dash165::control (M::note, 60, 0.0f), 0.0f, 0, m) == 1 && m[0].bytes[0] == 0x80);
    // a range: value 0..1 → low…high, rounded; an inverted range works too
    CHECK (yw::surface::midiValueOf (dash165::control (M::cc, 7, 0.0f, 20, 100), 0.5f) == 60);
    CHECK (yw::surface::midiValueOf (dash165::control (M::cc, 7, 0.0f, 100, 20), 1.0f) == 20);
    CHECK (yw::surface::midiValueOf (dash165::control (M::cc, 7, 0.0f), std::nanf ("")) == 0);
}

TEST (dashboard165_midi_settings_state)
{
    // a new Dashboard: YW's short state (FL's header), read back the same
    std::vector<std::uint8_t> st;
    yw::surface::DashboardMidi s;
    s.channel = 3;
    s.port = 7;
    s.program = 29;
    s.bankMsb = 84;
    s.bankLsb = -1;
    yw::surface::writeDashboardMidi (st, s);
    CHECK (yw::surface::dashboardMidi (st) == s);
    CHECK (yw::surface::dashboardMidi ({}) == yw::surface::DashboardMidi {});
    // FL's XP-30 preset: channel 1, port 0, bank 84 / 4, program 29; its six sliders send CC 74, 71, 73, 72, 91, 93
    if (const auto fl = dash164f::presetState ("XP30.fst"); ! fl.empty())
    {
        const auto m = yw::surface::dashboardMidi (fl);
        CHECK (m.channel == 0 && m.port == 0 && m.bankMsb == 84 && m.bankLsb == 4 && m.program == 29);
        const auto d = yw::surface::parseFlDashboard (fl);
        CHECK (d && d->controls.size() == 6);
        if (d && d->controls.size() == 6)
        {
            const int ccs[] = { 74, 71, 73, 72, 91, 93 };
            for (int k = 0; k < 6; ++k)
                CHECK (d->controls[(std::size_t) k].midi == yw::SurfaceControl::Midi::cc && d->controls[(std::size_t) k].midiNumber == ccs[k]
                       && d->controls[(std::size_t) k].midiHigh == 127);
        }
        // writing settings into FL's state keeps the rest of it
        auto copy = fl;
        auto changed = m;
        changed.port = 12;
        yw::surface::writeDashboardMidi (copy, changed);
        CHECK (copy.size() == fl.size() && yw::surface::dashboardMidi (copy).port == 12);
        CHECK (yw::surface::parseFlDashboard (copy).has_value());
    }
}

TEST (dashboard165_instrument_sends_its_midi)
{
    using M = yw::SurfaceControl::Midi;
    yw::surface::DashboardMidi s;
    s.channel = 1;
    s.program = 5;
    s.bankMsb = 2;
    auto c = dash165::dashboard ({ dash165::control (M::cc, 74, 0.5f), dash165::control (M::none, 0, 0.25f), dash165::control (M::note, 60, 1.0f) }, s);
    auto inst = yw::surface::makeDashboard();
    inst->prepare (44100.0);
    inst->configure (c, 120.0);
    // at load: bank, program, then each MIDI control's value (FL sends them when it loads)
    auto sent = dash165::take (*inst);
    CHECK (sent.size() == 4);
    if (sent.size() == 4)
    {
        CHECK ((sent[0] == std::vector<std::uint8_t> { 0xB1, 0, 2 }));
        CHECK ((sent[1] == std::vector<std::uint8_t> { 0xC1, 4 }));
        CHECK ((sent[2] == std::vector<std::uint8_t> { 0xB1, 74, 64 }));
        CHECK ((sent[3] == std::vector<std::uint8_t> { 0x91, 60, 127 }));
    }
    CHECK (std::abs (inst->controlOutput (1) - 0.25f) < 1e-6f && std::abs (inst->controlOutput (0) - 0.5f) < 1e-6f);
    // nothing changed: nothing more
    inst->configure (c, 120.0);
    CHECK (dash165::take (*inst).empty());
    // automation on output 0 → a new CC value; the same 7-bit value again sends nothing
    inst->setParameter (0, 1.0f);
    sent = dash165::take (*inst);
    CHECK (sent.size() == 1 && sent[0] == (std::vector<std::uint8_t> { 0xB1, 74, 127 }));
    inst->setParameter (0, 0.999f);
    CHECK (dash165::take (*inst).empty());
    CHECK (std::abs (inst->controlOutput (0) - 0.999f) < 1e-6f);
    // a controller-only control moves: its output, no MIDI
    inst->setParameter (1, 0.75f);
    CHECK (dash165::take (*inst).empty() && std::abs (inst->controlOutput (1) - 0.75f) < 1e-6f);
    // the song changes the note control to 0: note off
    c.patcher.surface[2].value = 0.0f;
    inst->configure (c, 120.0);
    sent = dash165::take (*inst);
    CHECK (sent.size() == 1 && sent[0] == (std::vector<std::uint8_t> { 0x81, 60, 0 }));
    // a new program: bank + program again
    s.program = 9;
    yw::surface::writeDashboardMidi (c.pluginState, s);
    inst->configure (c, 120.0);
    sent = dash165::take (*inst);
    CHECK (sent.size() >= 2 && sent[1] == (std::vector<std::uint8_t> { 0xC1, 8 }));
}

TEST (dashboard165_many_controls)
{
    using M = yw::SurfaceControl::Midi;
    // FL's Peavey preset: all 36 value controls now (the 32-output limit is gone)
    if (const auto st = dash164f::presetState ("Peavey.fst"); ! st.empty())
    {
        const auto d = yw::surface::parseFlDashboard (st);
        CHECK (d && d->controls.size() == 36);
        if (d && d->controls.size() == 36)
        {
            CHECK (d->controls[35].name == "Stereo M" && d->flOutputs[35] == 17);
            CHECK (yw::surface::flDashboardOutputToYw (st, 17) == 35);
        }
    }
    // 300 CC controls: every one sends (split over several audio blocks), every output reads back
    std::vector<yw::SurfaceControl> many;
    for (int k = 0; k < 300; ++k)
        many.push_back (dash165::control (M::cc, k % 128, (float) (k % 128) / 127.0f));
    auto c = dash165::dashboard (many, {});
    auto inst = yw::surface::makeDashboard();
    inst->prepare (44100.0);
    inst->configure (c, 120.0);
    const auto sent = dash165::take (*inst);
    CHECK (sent.size() == 300);
    if (sent.size() == 300)
        CHECK (sent[299][1] == 299 % 128 && sent[299][2] == 299 % 128);
    CHECK (std::abs (inst->controlOutput (299) - (float) (299 % 128) / 127.0f) < 1e-6f);
    CHECK (inst->controlOutput (1024) == 0.0f);
    // a Control Surface stays within one effect slot's parameters
    CHECK (yw::surface::maxSurfaceOutputs == yw::maxEffectParams && yw::surface::maxOutputs == 1024);
    CHECK ((int) yw::effectInfo (yw::EffectKind::controlSurface).params.size() == yw::maxEffectParams);
    auto slot = yw::makeEffectSlot (yw::EffectKind::controlSurface);
    slot.patcher.surface.clear();
    int added = 0;
    yw::SurfaceControl k;
    while (yw::surface::add (slot, k) >= 0 && added < 1000)
        ++added;
    CHECK (added == yw::maxEffectParams);
}

TEST (dashboard165_and_voltage_targets)
{
    yw::Song s;
    s.mixer = yw::Song::defaultMixer (2);
    yw::SurfaceControl a, b;
    a.kind = yw::SurfaceControl::Kind::knob;
    a.value = 0.2f;
    b.kind = yw::SurfaceControl::Kind::xy;
    b.value = 0.3f;
    b.valueY = 0.6f;
    s.channels.push_back (dash165::dashboard ({ a, b }, {}));
    yw::ChannelDef v;
    v.instrument = yw::InstrumentKind::voltage;
    v.pluginState = yw::voltage::write (*yw::voltage::parse (volt164::flState));
    s.channels.push_back (v);
    using K = yw::AutomationTargetKind;
    // Dashboard output 2 = the XY pad's Y
    CHECK (std::abs (s.targetValue ({ K::channelParam, 0, 0, 2 }) - 0.6f) < 1e-6f);
    CHECK (s.setTargetValue ({ K::channelParam, 0, 0, 1 }, 0.9f) && std::abs (s.channels[0].patcher.surface[1].value - 0.9f) < 1e-6f);
    CHECK (! s.setTargetValue ({ K::channelParam, 0, 0, 3 }, 0.9f));
    // Voltage Controller: 0…7 offsets, 8…15 scales (FL's numbers); flState's main R scale = 49152
    CHECK (std::abs (s.targetValue ({ K::channelParam, 1, 0, 9 }) - 0.75f) < 1e-6f);
    CHECK (s.setTargetValue ({ K::channelParam, 1, 0, 0 }, 0.25f));
    CHECK (yw::voltage::parse (s.channels[1].pluginState)->params[0] == 16384);
    CHECK (yw::voltage::paramName (0) == "Main L offset" && yw::voltage::paramName (15) == "Out 4 R scale" && yw::voltage::paramName (16).empty());
}

TEST (voltage165_linked_offset_moves_the_output)
{
    // FL's render of fl_dash.flp (REA_rodada164f-flimport §1.2): an offset linked to 0.25 gives −0.5 with the source off
    auto st = *yw::voltage::parse (volt164::flState);
    st.source[0] = yw::voltage::Source::none;
    volt164::Rig rig (st);
    rig.instrument->setParameter (0, 0.25f);
    rig.run (64);
    CHECK (std::abs (rig.left[32] + 0.5f) < 1e-4f);
    rig.instrument->setParameter (0, 0.875f);
    rig.run (64);
    CHECK (std::abs (rig.left[32] - 0.75f) < 1e-4f);
    // the scale too: main R = gate × scale; scale param 0.5 = × 0
    rig.instrument->setParameter (9, 0.5f);
    rig.instrument->noteOn (60, 1.0f);
    rig.run (64);
    CHECK (std::abs (rig.right[32]) < 1e-4f);
    rig.instrument->setParameter (-1, 0.5f); // (ignored)
    rig.instrument->setParameter (16, 0.5f);
    // the knob moved by hand (the song's value changes): it wins over the last automated value
    st.params[0] = 32768 + 16384; // +0.5
    yw::ChannelDef c;
    c.instrument = yw::InstrumentKind::voltage;
    c.pluginState = yw::voltage::write (st);
    rig.instrument->configure (c, 120.0);
    rig.run (64);
    CHECK (std::abs (rig.left[32] - 0.5f) < 1e-4f);
    // an unchanged song keeps a later automated value
    rig.instrument->setParameter (0, 0.25f);
    rig.instrument->configure (c, 120.0);
    rig.run (64);
    CHECK (std::abs (rig.left[32] + 0.5f) < 1e-4f);
}

TEST (dashboard165_links_and_midi_through_the_rack_in_pattern_mode)
{
    // FL's fl_dash.flp render (REA_rodada164f-flimport §1.2): a Dashboard's outputs linked to the Voltage Controller's
    // offsets L / R (sources off) give L = −0.5, R = +0.75 — also while a pattern plays (pattern mode): live links were
    // applied in song mode only before round 165
    for (const auto mode : { yw::PlayMode::pattern, yw::PlayMode::song })
    {
        yw::Song s;
        s.mixer = yw::Song::defaultMixer (2);
        auto st = *yw::voltage::parse (volt164::flState);
        st.source[0] = st.source[1] = yw::voltage::Source::none;
        st.params[9] = 65536;
        yw::ChannelDef v;
        v.instrument = yw::InstrumentKind::voltage;
        v.pluginState = yw::voltage::write (st);
        s.channels.push_back (v);
        using M = yw::SurfaceControl::Midi;
        s.channels.push_back (dash165::dashboard ({ dash165::control (M::none, 0, 0.25f), dash165::control (M::cc, 74, 0.875f) }, {}));
        yw::ChannelDef p;
        p.instrument = yw::InstrumentKind::plugin;
        p.plugin.id = "VST3-Synth-1";
        p.plugin.midiInPort = 0; // (the Dashboard's port)
        s.channels.push_back (p);
        s.patterns.emplace_back();
        s.mode = mode;
        for (int k = 0; k < 2; ++k)
        {
            yw::Automation a;
            a.target = { yw::AutomationTargetKind::channelParam, 0, 0, k };
            a.source.channel = 1;
            a.source.output = k;
            s.automations.push_back (a);
        }
        auto recorder = std::make_shared<feat133::Recorder>();
        yw::RackState::Factories f;
        f.pluginInstruments = [&] (const yw::PluginRef&, double) -> std::shared_ptr<yw::Instrument> { return recorder; };
        auto state = yw::RackState::build (s, nullptr, 48000.0, f);
        yw::RackRenderer r;
        r.prepare (48000.0, 256);
        std::vector<float> l (256), rr (256);
        for (int b = 0; b < 4; ++b)
            r.process (state.get(), true, (double) b * 10.0, 256, l.data(), rr.data(), nullptr);
        std::printf ("  rack (%s mode): L %.4f R %.4f\n", mode == yw::PlayMode::pattern ? "pattern" : "song", l[128], rr[128]);
        CHECK (std::abs (l[128] + 0.5f) < 2e-3f && std::abs (rr[128] - 0.75f) < 2e-3f);
        // the CC control's MIDI reached the plugin on the port (CC 74 = round (0.875 × 127) = 111)
        const auto cc = std::any_of (recorder->messages.begin(), recorder->messages.end(), [] (const auto& m) {
            return m.size() == 3 && m[0] == 0xB0 && m[1] == 74 && m[2] == 111;
        });
        CHECK (cc);
    }
}
