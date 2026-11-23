// --- Hardware output targets of the mixer (core/HardwareOutput.h, RackRenderer) - round 156 ----------------------

TEST (hwout_targets_pairs_then_monos)
{
    const auto four = yw::outputTargets ({ "Out 1", "Out 2", "Out 3", "Out 4" });
    CHECK (four.size() == 6);
    CHECK (four.size() == 6 && four[0].channel == 0 && four[0].mode == yw::OutputMode::stereo && four[0].name == "Out 1 - Out 2");
    CHECK (four.size() == 6 && four[1].channel == 2 && four[1].mode == yw::OutputMode::stereo);
    CHECK (four.size() == 6 && four[2].channel == 0 && four[2].mode == yw::OutputMode::monoSum && four[2].name == "Out 1");
    CHECK (four.size() == 6 && four[5].channel == 3);
    const auto odd = yw::outputTargets ({ "L", "R", "Aux" });
    CHECK (odd.size() == 4 && odd[3].name == "Aux");
    CHECK (yw::outputTargets ({}).empty());

    // names; a channel the device doesn't have is "(missing)"
    const std::vector<std::string> names { "Out 1", "Out 2", "Out 3", "Out 4" };
    CHECK (yw::outputTargetName (yw::outputDefault, yw::OutputMode::stereo, names, true) == "Out 1 - Out 2");
    CHECK (yw::outputTargetName (yw::outputDefault, yw::OutputMode::stereo, names, false) == "(none)");
    CHECK (yw::outputTargetName (2, yw::OutputMode::monoSum, names, false) == "Out 3 (mono)");
    CHECK (yw::outputTargetName (3, yw::OutputMode::monoRight, names, false) == "Out 4 (mono, R)");
    CHECK (yw::outputTargetName (3, yw::OutputMode::stereo, names, false) == "Out 4 - Out 5 (missing)"); // pair 4-5: no 5
    CHECK (yw::outputTargetName (6, yw::OutputMode::monoLeft, names, false) == "Out 7 (mono, L) (missing)");
    CHECK (yw::outputAvailable (2, yw::OutputMode::stereo, 4) && ! yw::outputAvailable (3, yw::OutputMode::stereo, 4));
    CHECK (! yw::outputAvailable (-1, yw::OutputMode::monoSum, 4));

    // defaults: Master = first pair, inserts = none (through the Master only)
    CHECK (yw::MixerTrack {}.outputChannel == yw::outputDefault && yw::MixerTrack {}.outputMode == yw::OutputMode::stereo);
}

TEST (hwout_routing_math)
{
    const std::vector<float> l { 0.5f, 1.0f }, r { -0.5f, 0.25f };
    std::vector<float> a (2, 0.0f), b (2, 0.0f);

    yw::addToOutput (l.data(), r.data(), yw::OutputMode::stereo, a.data(), b.data(), 2);
    CHECK (a[0] == 0.5f && a[1] == 1.0f && b[0] == -0.5f && b[1] == 0.25f);

    std::fill (a.begin(), a.end(), 0.0f);
    yw::addToOutput (l.data(), r.data(), yw::OutputMode::monoSum, a.data(), nullptr, 2);
    CHECK (a[0] == 0.0f && std::abs (a[1] - 0.625f) < 1e-7f); // (L+R)/2

    std::fill (a.begin(), a.end(), 0.0f);
    yw::addToOutput (l.data(), r.data(), yw::OutputMode::monoLeft, a.data(), nullptr, 2);
    CHECK (a[0] == 0.5f && a[1] == 1.0f);
    yw::addToOutput (l.data(), r.data(), yw::OutputMode::monoRight, a.data(), nullptr, 2); // adds, never overwrites
    CHECK (a[0] == 0.0f && a[1] == 1.25f);

    // a centred mono source keeps its level on a mono-sum output
    const std::vector<float> c { 0.3f };
    std::vector<float> m (1, 0.0f);
    yw::addToOutput (c.data(), c.data(), yw::OutputMode::monoSum, m.data(), nullptr, 1);
    CHECK (std::abs (m[0] - 0.3f) < 1e-7f);

    yw::addToOutput (nullptr, r.data(), yw::OutputMode::stereo, a.data(), b.data(), 2); // nothing to add: no crash
    yw::addToOutput (l.data(), r.data(), yw::OutputMode::stereo, nullptr, nullptr, 2);
    CHECK (a[1] == 1.25f);
}

TEST (hwout_renderer_feeds_buses_and_master)
{
    auto song = yw::Song::makeDefault();
    song.channels.clear();
    song.playlist.clear();
    song.mixer = yw::Song::defaultMixer (5);
    for (int t : { 2, 3, 4 })
        song.mixer[(std::size_t) t].monitor = yw::InputMonitor::on;
    song.mixer[2].inputChannel = 0;                    // stereo input 1-2: L 0.25, R -0.5
    song.mixer[2].outputChannel = 2;                   // -> Out 3, mono sum
    song.mixer[2].outputMode = yw::OutputMode::monoSum;
    song.mixer[3].inputChannel = 2;                    // mono input 3: 0.75 on both sides
    song.mixer[3].inputStereo = false;
    song.mixer[3].outputChannel = 2;                   // -> Out 3 too, left only (adds)
    song.mixer[3].outputMode = yw::OutputMode::monoLeft;
    song.mixer[4].inputChannel = 0;                    // -> Out 5 - Out 6 (stereo); the device has no Out 6
    song.mixer[4].outputChannel = 4;
    auto state = yw::RackState::build (song, nullptr, 48000.0);
    CHECK (state->hwRoutes.size() == 3);
    for (const auto& route : state->hwRoutes)
        CHECK (route.delay == nullptr); // no latency anywhere: nothing to line up

    yw::RackRenderer renderer;
    renderer.prepare (48000.0, 512);
    yw::LiveInput input;
    yw::HardwareOutput hw;
    renderer.setLiveInput (&input);
    renderer.setHardwareOutput (&hw);

    std::vector<float> a (256, 0.25f), b (256, -0.5f), c (256, 0.75f), l (256), r (256);
    input.channels[0] = a.data();
    input.channels[1] = b.data();
    input.channels[2] = c.data();
    input.numChannels = 3;

    // not active (no device / an export): nothing reaches the buses
    std::vector<std::vector<float>> outs (6, std::vector<float> (256, 0.0f));
    std::array<float*, 6> ptrs {};
    for (int k = 0; k < 6; ++k)
        ptrs[(std::size_t) k] = outs[(std::size_t) k].data();
    auto slotOf = [] (int ch) { return ch < 5 ? ch : -1; }; // Out 6 is not open
    renderer.process (state.get(), false, 0.0, 256, l.data(), r.data());
    hw.drainInto (ptrs.data(), 6, slotOf, 256);
    CHECK (outs[2][100] == 0.0f);

    hw.active = true;
    renderer.process (state.get(), false, 0.0, 256, l.data(), r.data());
    // the inserts still reach the Master (FL: the output target is a parallel path)
    CHECK (std::abs (l[100] - (0.25f + 0.75f + 0.25f)) < 1e-3f);
    hw.drainInto (ptrs.data(), 6, slotOf, 256);
    CHECK (std::abs (outs[2][100] - ((0.25f - 0.5f) * 0.5f + 0.75f)) < 1e-3f); // sum of insert 2 + left of insert 3
    CHECK (std::abs (outs[4][100] - 0.25f) < 1e-3f);                         // pair 5-6: left on Out 5
    CHECK (outs[5][100] == 0.0f && outs[0][100] == 0.0f && outs[1][100] == 0.0f); // missing Out 6 dropped; 1-2 untouched

    // the buses were cleared: a second drain adds nothing
    hw.drainInto (ptrs.data(), 6, slotOf, 256);
    CHECK (std::abs (outs[4][100] - 0.25f) < 1e-3f);

    // a muted insert sends nothing to its output either
    song.mixer[4].muted = true;
    auto muted = yw::RackState::build (song, state.get(), 48000.0);
    for (int i = 0; i < 12; ++i) // past the fader ramp (0.001 per sample)
    {
        renderer.process (muted.get(), false, 0.0, 256, l.data(), r.data());
        std::fill (outs[4].begin(), outs[4].end(), 0.0f);
        hw.drainInto (ptrs.data(), 6, slotOf, 256);
    }
    CHECK (std::abs (outs[4][100]) < 1e-6f);

    // a target past the buses (device with 100 outputs, say) is ignored, not a crash
    song.mixer[2].outputChannel = 200;
    auto far = yw::RackState::build (song, muted.get(), 48000.0);
    CHECK (far->hwRoutes.size() == 2);
    renderer.process (far.get(), false, 0.0, 256, l.data(), r.data());
    CHECK (hw.bus (64, 0, 1) == nullptr && hw.bus (0, yw::HardwareOutput::capacity, 1) == nullptr);
    hw.clear();
}

TEST (hwout_flp_import)
{
    // 0x93 after an insert's 0xEC: Master 0 (first pair, the default), insert 1 = 0 (direct to 1-2), insert 2 = −1
    FlpBuilder b;
    auto insert = [&b] (std::int32_t output) {
        b.data (0xEC, std::vector<std::uint8_t> (12, 0));
        b.u8 (0x93);
        b.u32 ((std::uint32_t) output);
    };
    insert (0);
    insert (0);
    insert (-1);
    const auto p = flp::parseProject (b.bytes);
    CHECK (p.mixer.size() == 3 && p.mixer[0].output == 0 && p.mixer[1].output == 0 && p.mixer[2].output == -1);
    const auto song = yw::importFlp (p);
    CHECK (song.mixer.size() > 2 && song.mixer[0].outputChannel == yw::outputDefault);
    CHECK (song.mixer.size() > 2 && song.mixer[1].outputChannel == 0 && song.mixer[1].outputMode == yw::OutputMode::stereo);
    CHECK (song.mixer.size() > 2 && song.mixer[2].outputChannel == yw::outputDefault);
}
