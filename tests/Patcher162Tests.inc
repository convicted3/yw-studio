// Round 162 (issue #7): the Patcher's I/O nodes' MIDI ports, MIDI Out and controllers inside a Patcher

namespace patcher162
{
yw::PatchWire midi (int from, int fromPort, int to, int toPort)
{
    yw::PatchWire w;
    w.kind = yw::PatchWire::Kind::midi;
    w.from = from;
    w.fromPort = fromPort;
    w.to = to;
    w.toPort = toPort;
    return w;
}

yw::PatchNode generatorNode (yw::InstrumentKind kind)
{
    yw::PatchNode n;
    n.generator = true;
    n.channel.instrument = kind;
    return n;
}

yw::PatchNode midiOutNode (int midiChannel, int port = 0)
{
    auto n = generatorNode (yw::InstrumentKind::midiOut);
    yw::midictl::MidiOutSettings s;
    s.channel = midiChannel;
    s.port = port;
    yw::midictl::writeMidiOut (n.channel.pluginState, s);
    return n;
}

double energy (yw::Instrument& i, int n = 2048)
{
    std::vector<float> l ((std::size_t) n, 0.0f), r ((std::size_t) n, 0.0f);
    i.beginChunk (n);
    i.render (l.data(), r.data(), n, 1.0f, 1.0f);
    double e = 0.0;
    for (const auto v : l)
        e += (double) v * v;
    return e;
}
} // namespace patcher162

TEST (patcher_from_yw_midi_port_pins_take_only_their_port)
{
    using namespace patcher162;
    yw::ChannelDef channel;
    channel.instrument = yw::InstrumentKind::patcher;
    channel.patcher.nodes = { generatorNode (yw::InstrumentKind::synth) };
    channel.patcher.wires = { midi (yw::patchFromHost, yw::patchMidiPortPin (3), 0, 0), patcher141::audio (0, yw::patchToHost) };
    auto play = [&] (auto&& start) {
        auto i = yw::patcher::makeInstrument (channel, 48000.0, yw::patcher::builtInFactories (48000.0));
        start (*i);
        return energy (*i);
    };
    CHECK (play ([] (yw::Instrument& i) { i.noteOn (60, 0.8f); }) == 0.0);              // the channel's own notes: not cabled
    CHECK (play ([] (yw::Instrument& i) { i.noteOnPort (2, 60, 0.8f, 0); }) == 0.0);    // another port
    CHECK (play ([] (yw::Instrument& i) { i.noteOnPort (3, 60, 0.8f, 0); }) > 1e-3);    // its port
    CHECK (yw::patcher::midiInputPorts (channel.patcher) == std::vector<int> { 3 });
}

TEST (patcher_midi_out_node_routes_notes_to_ports_with_its_channel)
{
    using namespace patcher162;
    yw::PatcherDoc doc;
    doc.nodes = { midiOutNode (5) };
    doc.wires = { midi (yw::patchFromHost, 0, 0, 0), midi (0, 0, yw::patchToHost, 7), midi (yw::patchFromHost, 0, yw::patchToHost, 2) };
    const auto routes = yw::patcher::midiOutputsOf (doc, 0);
    CHECK (routes.size() == 2);
    CHECK (std::find (routes.begin(), routes.end(), yw::patcher::MidiPortRoute { 7, 5 }) != routes.end());
    CHECK (std::find (routes.begin(), routes.end(), yw::patcher::MidiPortRoute { 2, -1 }) != routes.end());
    // a MIDI Out node has a notes input and a MIDI output, no audio
    const auto ports = yw::patcher::nodePorts (doc.nodes[0]);
    CHECK (ports.midiIn && ports.midiOut && ports.audioOuts == 0 && ports.controlOuts == 0);
    // a disabled cable passes nothing
    doc.wires[1].enabled = false;
    CHECK (yw::patcher::midiOutputsOf (doc, 0).size() == 1);
    // MIDI cables can't loop on a node
    CHECK (! yw::patcher::connect (doc, midi (0, 0, 0, 0)));
}

TEST (patcher_controller_nodes_have_their_outputs)
{
    using namespace patcher162;
    auto ec = generatorNode (yw::InstrumentKind::controller);
    ec.channel.controllerState = yw::writeEnvelopeController (yw::defaultEnvelopeController());
    auto ports = yw::patcher::nodePorts (ec);
    CHECK (ports.controlOuts == 8 && ports.midiIn && ports.audioOuts == 0);

    // an Envelope Controller inside a generator Patcher moves a node's parameter through a control cable
    yw::ChannelDef channel;
    channel.instrument = yw::InstrumentKind::patcher;
    channel.patcher.nodes = { ec, patcher141::effectNode (yw::EffectKind::utility) };
    yw::PatchWire control;
    control.kind = yw::PatchWire::Kind::control;
    control.from = 0;
    control.fromPort = 0;
    control.to = 1;
    control.toPort = 0;
    channel.patcher.wires = { midi (yw::patchFromHost, 0, 0, 0), control };
    auto i = yw::patcher::makeInstrument (channel, 48000.0, yw::patcher::builtInFactories (48000.0));
    auto* graph = yw::patcher::graphOf (i.get());
    CHECK (graph != nullptr);
    if (graph == nullptr)
        return;
    i->noteOn (60, 1.0f);
    energy (*i, 1024); // (the envelope rises: 20 ms attack)
    const auto ecNode = graph->instrumentAt (0);
    CHECK (ecNode != nullptr && ecNode->controlOutput (0) > 0.5f);
}

TEST (patcher_io_midi_pins_show_and_deactivate)
{
    using namespace patcher162;
    yw::PatcherDoc doc;
    doc.fromMidiPorts = { 4, 9 };
    doc.toMidiPorts = { 1 };
    doc.nodes = { generatorNode (yw::InstrumentKind::synth) };
    doc.wires = { midi (yw::patchFromHost, yw::patchMidiPortPin (9), 0, 0), midi (yw::patchFromHost, yw::patchMidiPortPin (12), 0, 0),
                  midi (yw::patchFromHost, 0, yw::patchToHost, 3) };
    CHECK ((yw::patcher::shownFromMidiPorts (doc) == std::vector<int> { 4, 9, 12 }));
    CHECK ((yw::patcher::shownToMidiPorts (doc) == std::vector<int> { 1, 3 }));
    CHECK (yw::patcher::fromNotesShown (doc, false)); // (cabled, even in a mixer slot)
    yw::patcher::deactivateUnusedMidiPins (doc, true);
    CHECK ((doc.fromMidiPorts == std::vector<int> { 9 }) && ! doc.fromNotesHidden);
    yw::patcher::deactivateUnusedMidiPins (doc, false);
    CHECK (doc.toMidiPorts.empty());
    yw::PatcherDoc empty;
    CHECK (yw::patcher::fromNotesShown (empty, true) && ! yw::patcher::fromNotesShown (empty, false));
    empty.fromNotesHidden = true;
    CHECK (! yw::patcher::fromNotesShown (empty, true));
}

TEST (patcher_midi_out_channel_reaches_patchers_on_its_port)
{
    using namespace patcher162;
    // a MIDI Out channel on port 4 plays a generator Patcher whose "MIDI port 4" pin feeds a synth, and a mixer-slot
    // Patcher on insert 2 whose "MIDI port 4" pin feeds another synth
    auto song = yw::Song::makeDefault();
    song.mixer.resize (4);
    song.channels.resize (2);
    song.channels[0] = {};
    song.channels[0].instrument = yw::InstrumentKind::midiOut;
    yw::midictl::MidiOutSettings s;
    s.port = 4;
    s.channel = 2;
    yw::midictl::writeMidiOut (song.channels[0].pluginState, s);
    song.channels[0].mixerTrack = 1;
    auto& target = song.channels[1];
    target = {};
    target.instrument = yw::InstrumentKind::patcher;
    target.mixerTrack = 1;
    target.patcher.nodes = { generatorNode (yw::InstrumentKind::synth) };
    target.patcher.wires = { midi (yw::patchFromHost, yw::patchMidiPortPin (4), 0, 0), patcher141::audio (0, yw::patchToHost) };
    yw::Pattern p;
    p.notes.push_back ({ 0, yw::ppq, 0, 60, 0.8f });
    song.patterns = { p };
    yw::Clip clip;
    clip.pattern = 0;
    clip.length = yw::ppq * 4;
    song.playlist = { clip };

    auto render = [] (const yw::Song& sg) {
        auto state = yw::RackState::build (sg, nullptr, 48000.0);
        yw::RackRenderer r;
        r.prepare (48000.0, 512);
        std::vector<float> l (512), rr (512);
        double e = 0.0, tick = 0.0;
        for (int b = 0; b < 8; ++b, tick += 512.0 * sg.bpm * yw::ppq / 60.0 / 48000.0)
        {
            r.process (state.get(), true, tick, 512, l.data(), rr.data(), nullptr);
            for (const auto v : l)
                e += (double) v * v;
        }
        return e;
    };
    {
        auto state = yw::RackState::build (song, nullptr, 48000.0);
        CHECK (state->midiRoutesOut[0].size() == 1 && state->midiRoutesOut[0][0].other == 1 && state->midiRoutesOut[0][0].port == 4
               && state->midiRoutesOut[0][0].midiChannel == 2);
    }
    CHECK (render (song) > 1e-4);
    auto other = song;
    yw::midictl::MidiOutSettings s5 = s;
    s5.port = 5;
    yw::midictl::writeMidiOut (other.channels[0].pluginState, s5);
    CHECK (render (other) == 0.0); // another port: silence

    // the mixer-slot Patcher
    auto mixerSong = song;
    mixerSong.channels.resize (1);
    yw::EffectSlot slot = yw::makeEffectSlot (yw::EffectKind::patcher);
    slot.patcher.nodes = { generatorNode (yw::InstrumentKind::synth) };
    slot.patcher.wires = { midi (yw::patchFromHost, yw::patchMidiPortPin (4), 0, 0), patcher141::audio (0, yw::patchToHost),
                           patcher141::audio (yw::patchFromHost, yw::patchToHost) };
    mixerSong.mixer[2].effects = { slot };
    {
        auto state = yw::RackState::build (mixerSong, nullptr, 48000.0);
        CHECK (state->hasEffectMidiRoutes && state->effectMidiRoutes[0].size() == 1 && state->effectMidiRoutes[0][0].track == 2);
    }
    CHECK (render (mixerSong) > 1e-4);

    // a generator Patcher passing its port 4 on to "To YW" MIDI port 6, through a MIDI Out node (MIDI channel 9):
    // a plugin channel on port 6 is then a target of the MIDI Out channel
    auto chain = song;
    chain.channels[1].patcher.nodes.push_back (midiOutNode (9));
    chain.channels[1].patcher.wires.push_back (midi (yw::patchFromHost, yw::patchMidiPortPin (4), 1, 0));
    chain.channels[1].patcher.wires.push_back (midi (1, 0, yw::patchToHost, 6));
    chain.channels.push_back ({});
    chain.channels[2].instrument = yw::InstrumentKind::plugin;
    chain.channels[2].plugin.midiInPort = 6;
    {
        auto state = yw::RackState::build (chain, nullptr, 48000.0);
        const auto& routes = state->midiRoutesOut[0];
        CHECK (std::any_of (routes.begin(), routes.end(), [] (const yw::RackState::MidiRoute& r) { return r.other == 2 && r.port == 6 && r.midiChannel == 9; }));
    }
}

TEST (patcher_fl_midi_port_pins_import)
{
    using namespace patcher141;
    yw::PatcherPort pin;
    pin.kind = 1;
    pin.name = "Notes";
    CHECK (yw::flPatcherMidiPinPort (pin) == -1);
    pin.name = "MIDI port 17";
    CHECK (yw::flPatcherMidiPinPort (pin) == 17);
    pin.name = "My keyboard";
    pin.index = 33;
    CHECK (yw::flPatcherMidiPinPort (pin) == 33);
    pin.kind = 0;
    CHECK (yw::flPatcherMidiPinPort (pin) == -2);

    // a mixer Patcher: "From" pins MIDI port 5 (cabled) and MIDI port 8 (activated, no cable), "To" pin MIDI port 9;
    // a cable From MIDI port 5 → To MIDI port 9; and the audio through
    FlBlob blob;
    std::vector<std::uint8_t> balance;
    FlBlob::put (balance, (std::int32_t) 1);
    FlBlob::put (balance, (std::int32_t) 0);
    FlBlob::put (balance, (std::int32_t) 256);
    blob.node (2, 100, "Fruity Balance", balance, { { 0, 0, 1001, true }, { 0, 0, 1002, false } });
    blob.pin (2001, 0, true, 0, "Audio input");
    blob.pin (2002, 0, false, 0, "Audio output");
    blob.pin (2003, 1, true, 5, "MIDI port 5");
    blob.pin (2004, 1, true, 8, "MIDI port 8");
    blob.pin (2005, 1, false, 9, "MIDI port 9");
    blob.cable (1001, 2001, 1.0f, 0);
    blob.cable (2002, 1002, 1.0f, 0);
    blob.cable (2005, 2003, 1.0f, 1);
    const auto state = blob.finish();

    flp::Project project;
    project.tempo = 120.0;
    flp::MixerInsert insert;
    insert.index = 1;
    flp::MixerInsert::Slot s;
    s.index = 0;
    s.plugin = "Patcher";
    s.state = state;
    insert.slots.push_back (s);
    project.mixer.push_back (insert);
    const auto song = yw::importFlp (project);
    CHECK (song.mixer.size() > 1 && song.mixer[1].effects.size() == 1);
    if (song.mixer.size() < 2 || song.mixer[1].effects.empty())
        return;
    const auto& doc = song.mixer[1].effects[0].patcher;
    CHECK ((doc.fromMidiPorts == std::vector<int> { 5, 8 }));
    CHECK ((doc.toMidiPorts == std::vector<int> { 9 }));
    CHECK (std::any_of (doc.wires.begin(), doc.wires.end(), [] (const yw::PatchWire& w) {
        return w.kind == yw::PatchWire::Kind::midi && w.from == yw::patchFromHost && w.fromPort == yw::patchMidiPortPin (5)
               && w.to == yw::patchToHost && w.toPort == 9;
    }));
    CHECK ((yw::patcher::midiOutputsOf (doc, yw::patchMidiPortPin (5)) == std::vector<yw::patcher::MidiPortRoute> { { 9, -1 } }));
}
