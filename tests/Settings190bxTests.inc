// Round 190bx: FL's Settings options YW honours (core/FlSettings.h; docs/analise/REA_rodada190bx.md). One test per
// option with an effect on the engine or on an edit, plus the options file.
#include "core/FlSettings.h"

#include <thread>

namespace s190bx
{
// every test starts from FL's defaults and leaves them behind
struct Defaults
{
    Defaults() { yw::flsettings::reset(); yw::flsettings::forgetSolos(); }
    ~Defaults() { yw::flsettings::reset(); yw::flsettings::forgetSolos(); }
};

// a sampler channel playing one transposed note of a 1 kHz sine (a 44.1 kHz file on a 48 kHz device), 4800 frames
std::vector<float> samplerNote()
{
    yw::Song s;
    s.mixer = yw::Song::defaultMixer (1);
    yw::ChannelDef c;
    c.instrument = yw::InstrumentKind::sampler;
    c.samplePath = "sine";
    c.mixerTrack = 1;
    s.channels.push_back (c);
    s.patterns.emplace_back();
    yw::Note n;
    n.start = 0;
    n.length = 4 * yw::ticksPerStep;
    n.channel = 0;
    n.key = 67;
    s.patterns[0].notes.push_back (n);
    yw::RackState::Factories f;
    f.samples = [] (const std::string&) {
        auto d = std::make_shared<yw::SampleData>();
        d->sampleRate = 44100.0;
        for (int i = 0; i < 44100; ++i)
            d->left.push_back ((float) (0.5 * std::sin (2.0 * 3.141592653589793 * 1000.0 * (double) i / 44100.0)));
        return std::shared_ptr<const yw::SampleData> (d);
    };
    auto state = yw::RackState::build (s, nullptr, 48000.0, f);
    yw::RackRenderer r;
    r.prepare (48000.0, 512);
    std::vector<float> out, l (480), rr (480);
    const auto tps = yw::ticksPerSample (s.bpm, 48000.0);
    for (int block = 0; block < 10; ++block)
    {
        r.process (state.get(), true, block * 480 * tps, 480, l.data(), rr.data());
        out.insert (out.end(), l.begin(), l.end());
    }
    return out;
}

// records the threads its process() ran on
class ThreadSpy final : public yw::Effect
{
public:
    void prepare (double) noexcept override {}
    void reset() noexcept override {}
    void process (float*, float*, int, const yw::EffectParams&, double) noexcept override
    {
        if (std::this_thread::get_id() != home)
            foreign.fetch_add (1);
        calls.fetch_add (1);
    }
    std::thread::id home = std::this_thread::get_id();
    std::atomic<int> foreign { 0 }, calls { 0 };
};
} // namespace s190bx

TEST (settings190bx_options_file)
{
    using namespace yw::flsettings;
    const s190bx::Defaults guard;
    auto& o = options();
    // FL's defaults (the Settings form: 24-point sinc, the CPU boxes ticked, Auto close and Restore... unticked,
    // Auto name channels and Read sample tempo information ticked, the history oldest first)
    CHECK (o.interpolation == 4 && o.multithreadGenerators && o.multithreadMixer && o.smartDisable && ! o.autoCloseDevice);
    CHECK (! o.restoreStateAfterSolo && o.autoNameChannels && o.readSampleTempo && ! o.historyNewestFirst);
    CHECK (interpolationPoints (1) == 2 && interpolationPoints (2) == 6 && interpolationPoints (4) == 24 && interpolationPoints (9) == 512);
    CHECK (interpolationPoints (interpolationHeavyFrom) == 64);

    // every option round-trips through its line of ui-options.txt
    o.interpolation = 7;
    o.multithreadGenerators = false;
    o.multithreadMixer = false;
    o.smartDisable = false;
    o.autoCloseDevice = true;
    o.restoreStateAfterSolo = true;
    o.autoNameChannels = false;
    o.readSampleTempo = false;
    o.historyNewestFirst = true;
    const auto text = serialise();
    reset();
    std::size_t at = 0;
    int lines = 0;
    while (at < text.size())
    {
        const auto end = std::min (text.find ('\n', at + 1), text.size());
        const auto line = text.substr (at + 1, end - at - 1);
        const auto eq = line.find ('=');
        CHECK (eq != std::string::npos && parse (line.substr (0, eq), line.substr (eq + 1)));
        ++lines;
        at = end;
    }
    CHECK (lines == 9 + 5 + 1 + 1 + 1); // (round 190mr: + history on undo, data size warning, click-and-hold, both buttons, layout with project; 190mv: undo knob tweaks; 190ok: safe overloads, smart disable for new instrument plugins)
    CHECK (o.interpolation == 7 && ! o.multithreadGenerators && ! o.multithreadMixer && ! o.smartDisable && o.autoCloseDevice);
    CHECK (o.restoreStateAfterSolo && ! o.autoNameChannels && ! o.readSampleTempo && o.historyNewestFirst);

    // a key of another option is not taken; an unreadable value is FL's default
    CHECK (! parse ("metronomeTrack", "3") && ! parse ("flInterpolationX", "3"));
    CHECK (parse ("flInterpolation", "0") && o.interpolation == 4);
    CHECK (parse ("flInterpolation", "12") && o.interpolation == 4);
    CHECK (parse ("flInterpolation", "abc") && o.interpolation == 4);
    CHECK (parse ("flInterpolation", "9") && o.interpolation == 9);
    CHECK (parse ("flSmartDisable", "?") && o.smartDisable);
    CHECK (parse ("flAutoCloseDevice", "") && ! o.autoCloseDevice);
}

TEST (settings190bx_resampling_quality_reaches_the_sampler)
{
    const s190bx::Defaults guard;
    auto& quality = yw::flsettings::options().interpolation;
    const auto standard = s190bx::samplerNote(); // 24-point sinc, as before this round
    auto peak = 0.0f;
    for (const auto v : standard)
        peak = std::max (peak, std::abs (v));
    CHECK (peak > 0.05f);
    quality = 4;
    CHECK (s190bx::samplerNote() == standard);
    quality = 1;
    const auto linear = s190bx::samplerNote();
    quality = 2;
    const auto hermite = s190bx::samplerNote();
    quality = 9;
    const auto sinc512 = s190bx::samplerNote();
    CHECK (linear != standard && hermite != standard && sinc512 != standard && linear != hermite);
    // the same note, only read differently: the methods agree within a little (linear the least)
    auto distance = [&standard] (const std::vector<float>& other) {
        auto d = 0.0;
        for (std::size_t i = 0; i < standard.size(); ++i)
            d = std::max (d, (double) std::abs (standard[i] - other[i]));
        return d;
    };
    CHECK (distance (linear) < 0.05 && distance (sinc512) < 0.01);
    CHECK (distance (linear) > distance (sinc512));
    // the kernel sizes behind the choices
    for (int index = 1; index <= 9; ++index)
    {
        yw::flrs::Kernel k;
        k.setQuality (index);
        k.setSpeed (1.0);
        CHECK (2 * k.halfWidth == yw::flsettings::interpolationPoints (index));
    }
}

TEST (settings190bx_smart_disable_global_switch)
{
    const s190bx::Defaults guard;
    // 6 s of silence through an insert effect whose own "Smart disable" is on
    auto run = [] {
        yw::Song s;
        s.mixer = yw::Song::defaultMixer (1);
        yw::ChannelDef c;
        c.instrument = yw::InstrumentKind::sampler;
        c.samplePath = "dc";
        c.mixerTrack = 1;
        s.channels.push_back (c);
        s.patterns.emplace_back();
        yw::EffectSlot fx;
        fx.kind = yw::EffectKind::plugin;
        fx.plugin.id = "inversor";
        s.mixer[1].effects.push_back (fx);
        auto inverter = std::make_shared<CountingInverter>();
        yw::RackState::Factories f;
        f.samples = [] (const std::string&) {
            auto d = std::make_shared<yw::SampleData>();
            d->left.assign (4800, 0.5f);
            return std::shared_ptr<const yw::SampleData> (d);
        };
        f.pluginEffects = [inverter] (const yw::PluginRef&, double) -> std::shared_ptr<yw::Effect> { return inverter; };
        auto state = yw::RackState::build (s, nullptr, 48000.0, f);
        yw::RackRenderer r;
        r.prepare (48000.0, 512);
        std::vector<float> l (480), rr (480);
        for (int i = 0; i < 100 * 6; ++i)
            r.process (state.get(), false, 0.0, 480, l.data(), rr.data());
        return inverter->calls;
    };
    CHECK (run() < 100 * 6); // on (FL's default): the slot sleeps after its hold time
    yw::flsettings::options().smartDisable = false;
    CHECK (run() == 100 * 6); // off: processed in every block
}

TEST (settings190bx_multithreading_switches)
{
    const s190bx::Defaults guard;
    // a song busy enough to be rendered in parallel, with a spy effect on four inserts
    auto song = yw::Song::makeDefault();
    song.mixer = yw::Song::defaultMixer (16);
    for (int i = 5; i < 32; ++i)
    {
        yw::ChannelDef c;
        c.instrument = i % 2 ? yw::InstrumentKind::synth : yw::InstrumentKind::hat;
        c.mixerTrack = 1 + i % 12;
        c.volume = 0.3f;
        song.channels.push_back (c);
        for (int step = i % 4; step < 16; step += 3)
            song.patterns[0].notes.push_back ({ (std::int64_t) step * yw::ticksPerStep, 200, i, 48 + i, 0.7f });
    }
    song.patterns[0].sortNotes();
    for (int t = 1; t <= 8; ++t)
    {
        auto fx = yw::makeEffectSlot (yw::EffectKind::plugin);
        fx.plugin.id = "spy";
        fx.smartDisable = false;
        song.mixer[(std::size_t) t].effects.push_back (fx);
        song.mixer[(std::size_t) t].effects.push_back (yw::makeEffectSlot (yw::EffectKind::reverb));
    }

    struct Result
    {
        std::vector<float> audio;
        int foreign = 0, calls = 0;
    };
    auto render = [&song] (bool pool) {
        std::vector<std::shared_ptr<s190bx::ThreadSpy>> spies;
        yw::RackState::Factories f;
        f.pluginEffects = [&spies] (const yw::PluginRef&, double) -> std::shared_ptr<yw::Effect> {
            spies.push_back (std::make_shared<s190bx::ThreadSpy>());
            return spies.back();
        };
        auto state = yw::RackState::build (song, nullptr, 48000.0, f);
        yw::WorkerPool workers (3);
        yw::RackRenderer r;
        r.prepare (48000.0, 512);
        if (pool)
            r.setWorkerPool (&workers);
        Result result;
        std::vector<float> l (512), rr (512);
        const auto tps = yw::ticksPerSample (song.bpm, 48000.0);
        for (int block = 0; block < 120; ++block)
        {
            r.process (state.get(), true, block * 512 * tps, 512, l.data(), rr.data());
            result.audio.insert (result.audio.end(), l.begin(), l.end());
            result.audio.insert (result.audio.end(), rr.begin(), rr.end());
        }
        for (const auto& spy : spies)
        {
            result.foreign += spy->foreign.load();
            result.calls += spy->calls.load();
        }
        return result;
    };

    const auto alone = render (false); // no worker threads at all: the reference
    CHECK (alone.calls >= 8 * 120 && alone.foreign == 0);
    auto& o = yw::flsettings::options();
    for (const auto generators : { true, false })
        for (const auto mixer : { true, false })
        {
            o.multithreadGenerators = generators;
            o.multithreadMixer = mixer;
            const auto got = render (true);
            // the sound never depends on the switches
            CHECK (got.audio == alone.audio);
            CHECK (got.calls == alone.calls);
            // "Multithreaded mixer processing" off: every insert effect runs on the audio thread itself
            if (! mixer)
                CHECK (got.foreign == 0);
        }
}

TEST (settings190bx_auto_close_device)
{
    using namespace yw::flsettings;
    DeviceUse front, back, backBusy;
    back.foreground = false;
    backBusy.foreground = false;
    backBusy.busy = true;
    // off (FL's default): nothing ever
    CHECK (autoCloseAction (false, back, true, false) == DeviceAction::none);
    // on: the device closes only in the background, and only when nothing needs it
    CHECK (autoCloseAction (true, front, true, false) == DeviceAction::none);
    CHECK (autoCloseAction (true, back, true, false) == DeviceAction::close);
    CHECK (autoCloseAction (true, backBusy, true, false) == DeviceAction::none);
    // closed by the option: stays closed in the background, opens again in front, when needed, or when the option goes off
    CHECK (autoCloseAction (true, back, false, true) == DeviceAction::none);
    CHECK (autoCloseAction (true, front, false, true) == DeviceAction::reopen);
    CHECK (autoCloseAction (true, backBusy, false, true) == DeviceAction::reopen);
    CHECK (autoCloseAction (false, back, false, true) == DeviceAction::reopen);
    // a device that is closed for another reason (none chosen, a failure) is never opened from here
    CHECK (autoCloseAction (true, front, false, false) == DeviceAction::none);
    CHECK (autoCloseAction (true, back, false, false) == DeviceAction::none);
    // opened again by something else meanwhile: nothing to reopen
    CHECK (autoCloseAction (true, front, true, true) == DeviceAction::none);
}

TEST (settings190bx_restore_previous_state_after_solo)
{
    const s190bx::Defaults guard;
    using yw::flsettings::SoloMemory;
    auto song = yw::Song::makeDefault();
    while (song.channels.size() < 5)
        song.channels.push_back (song.channels[0]);
    auto set = [&song] (std::initializer_list<bool> muted) {
        std::size_t i = 0;
        for (const auto m : muted)
            song.channels[i++].muted = m;
        for (auto& c : song.channels)
            c.soloLock = false;
    };
    auto is = [&song] (std::initializer_list<bool> muted) {
        std::size_t i = 0;
        for (const auto m : muted)
            if (song.channels[i++].muted != m)
                return false;
        return true;
    };

    // option off (FL's default): the un-solo switches every channel on, with or without a memory
    {
        SoloMemory memory;
        set ({ false, true, false, true, false });
        yw::noteedit::soloChannel (song, 2, &memory, false);
        CHECK (is ({ true, true, false, true, true }));
        yw::noteedit::soloChannel (song, 2, &memory, false);
        CHECK (is ({ false, false, false, false, false }));
        CHECK (memory.row == -1);
    }
    // option on: the un-solo puts back what was on before
    {
        SoloMemory memory;
        set ({ false, true, false, true, false });
        yw::noteedit::soloChannel (song, 2, &memory, true);
        CHECK (is ({ true, true, false, true, true }));
        yw::noteedit::soloChannel (song, 2, &memory, true);
        CHECK (is ({ false, true, false, true, false }));
    }
    // a solo moved to another channel keeps the memory of before the first solo (FL takes it once)
    {
        SoloMemory memory;
        set ({ false, true, false, true, false });
        yw::noteedit::soloChannel (song, 2, &memory, true);
        yw::noteedit::soloChannel (song, 4, &memory, true);
        CHECK (is ({ true, true, true, true, false }));
        yw::noteedit::soloChannel (song, 4, &memory, true);
        CHECK (is ({ false, true, false, true, false }));
    }
    // the soloed channel was itself muted: it goes back to muted
    {
        SoloMemory memory;
        set ({ false, true, false, false, false });
        yw::noteedit::soloChannel (song, 1, &memory, true);
        CHECK (is ({ true, false, true, true, true }));
        yw::noteedit::soloChannel (song, 1, &memory, true);
        CHECK (is ({ false, true, false, false, false }));
    }
    // the option is read at the un-solo (FL): switched on while a solo is on, it still restores
    {
        SoloMemory memory;
        set ({ false, true, false, true, false });
        yw::noteedit::soloChannel (song, 0, &memory, false);
        yw::noteedit::soloChannel (song, 0, &memory, true);
        CHECK (is ({ false, true, false, true, false }));
    }
    // channels with "Lock active/mute state" are never touched, on the way in or back
    {
        SoloMemory memory;
        set ({ false, true, false, true, false });
        song.channels[4].soloLock = true;
        yw::noteedit::soloChannel (song, 0, &memory, true);
        CHECK (is ({ false, true, true, true, false }));
        song.channels[4].muted = true; // (by hand, during the solo)
        yw::noteedit::soloChannel (song, 0, &memory, true);
        CHECK (is ({ false, true, false, true, true }));
    }
    // a solo that came with the project (nothing remembered), or a memory of another row count: everything on
    {
        SoloMemory memory;
        set ({ true, true, false, true, true });
        yw::noteedit::soloChannel (song, 2, &memory, true);
        CHECK (is ({ false, false, false, false, false }));
        memory.muted = { true, false };
        memory.row = 2;
        set ({ true, true, false, true, true });
        yw::noteedit::soloChannel (song, 2, &memory, true);
        CHECK (is ({ false, false, false, false, false }));
    }
    // without a memory the calls are the ones of before this round
    {
        set ({ false, true, false, true, false });
        yw::noteedit::soloChannel (song, 2);
        CHECK (is ({ true, true, false, true, true }));
        yw::noteedit::soloChannel (song, 2);
        CHECK (is ({ false, false, false, false, false }));
    }

    // Playlist tracks: the same memory
    {
        yw::Song s;
        SoloMemory memory;
        yw::noteedit::soloTrack (s, 3, 6);
        yw::noteedit::soloTrack (s, 3, 6); // (all on; six headers now)
        s.playlistTracks[1].muted = true;
        s.playlistTracks[5].muted = true;
        yw::noteedit::soloTrack (s, 3, 6, &memory, true);
        for (int t = 0; t < 6; ++t)
            CHECK (s.playlistTracks[(std::size_t) t].muted == (t != 3));
        yw::noteedit::soloTrack (s, 3, 6, &memory, true);
        for (int t = 0; t < 6; ++t)
            CHECK (s.playlistTracks[(std::size_t) t].muted == (t == 1 || t == 5));
        yw::noteedit::soloTrack (s, 3, 6, &memory, false);
        yw::noteedit::soloTrack (s, 3, 6, &memory, false);
        for (int t = 0; t < 6; ++t)
            CHECK (! s.playlistTracks[(std::size_t) t].muted);
    }

    // the app's two memories are forgotten when another project loads
    yw::flsettings::channelSoloMemory().row = 1;
    yw::flsettings::trackSoloMemory().row = 2;
    yw::flsettings::forgetSolos();
    CHECK (yw::flsettings::channelSoloMemory().row == -1 && yw::flsettings::trackSoloMemory().row == -1);
}

TEST (settings190bx_auto_name_channels)
{
    yw::ChannelDef c;
    c.instrument = yw::InstrumentKind::sampler;
    c.name = "Lead";
    // on (FL's default, and the call of before this round): the channel takes the file's name
    yw::audioclips::loadSampleInto (c, "a/kick.wav", "kick");
    CHECK (c.name == "kick" && c.samplePath == "a/kick.wav");
    // off: a named channel keeps its name, the sample still loads
    yw::audioclips::loadSampleInto (c, "a/snare.wav", "snare", false);
    CHECK (c.name == "kick" && c.samplePath == "a/snare.wav" && c.instrument == yw::InstrumentKind::sampler);
    // off, a channel without a name: it still gets one
    yw::ChannelDef fresh;
    yw::audioclips::loadSampleInto (fresh, "a/hat.wav", "hat", false);
    CHECK (fresh.name == "hat");
}

TEST (settings190bx_read_sample_tempo_information)
{
    const s190bx::Defaults guard;
    CHECK (yw::flsettings::sampleTempoBeats (8.0f) == 8.0f);
    CHECK (yw::flsettings::sampleTempoBeats (0.0f) == 0.0f);
    yw::flsettings::options().readSampleTempo = false;
    CHECK (yw::flsettings::sampleTempoBeats (8.0f) == 0.0f);
    // what follows from it: a loop whose file says "8 beats" is fitted by its length alone
    const auto withInfo = yw::clipops::fitBeats (4.0, 120.0, 16.0f);
    const auto without = yw::clipops::fitBeats (4.0, 120.0, yw::flsettings::sampleTempoBeats (16.0f));
    CHECK (withInfo == 16.0f);
    CHECK (without != 16.0f);
}

TEST (settings190bx_history_newest_first)
{
    using namespace yw::flsettings;
    for (int row = 0; row < 5; ++row)
    {
        CHECK (historyStepForRow (row, 5, false) == row && historyRowForStep (row, 5, false) == row);
        CHECK (historyStepForRow (row, 5, true) == 4 - row);
        CHECK (historyRowForStep (historyStepForRow (row, 5, true), 5, true) == row);
    }
}

TEST (settings190bx_reset_working_time)
{
    yw::ProjectInfo info;
    info.title = "Song";
    info.created = 1000.0;
    info.workSeconds = 7200.0;
    yw::resetWorkTime (info, 5000.0);
    // FL's "Reset working time": the project starts now; nothing else of the info changes
    CHECK (info.created == 5000.0 && info.workSeconds == 0.0 && info.title == "Song");
}
