// Round 181c: the wrapper options a plugin gets when it is created must not wait for a reload once they change.

namespace wrappermidi181c
{
struct Silent final : yw::Instrument
{
    void prepare (double) noexcept override {}
    void noteOn (int, float) noexcept override {}
    void noteOff (int) noexcept override {}
    void allNotesOff() noexcept override {}
    void render (float*, float*, int, float, float) noexcept override {}
};
struct Pass final : yw::Effect
{
    void prepare (double) noexcept override {}
    void reset() noexcept override {}
    void process (float*, float*, int, const yw::EffectParams&, double) noexcept override {}
};
inline yw::Song song()
{
    auto s = yw::Song::makeDefault();
    auto& c = s.channels[0];
    c.instrument = yw::InstrumentKind::plugin;
    c.plugin = {};
    c.plugin.id = "VST-Test";
    yw::EffectSlot slot;
    slot.kind = yw::EffectKind::plugin;
    slot.plugin.id = "VST-Effect";
    s.mixer[1].effects.clear();
    s.mixer[1].effects.push_back (slot);
    return s;
}
} // namespace wrappermidi181c

TEST (wrapper181c_midi_options_reach_a_new_instance_of_the_plugin)
{
    using namespace wrappermidi181c;
    int instruments = 0, effects = 0;
    yw::RackState::Factories f;
    f.pluginInstruments = [&] (const yw::PluginRef&, double) -> std::shared_ptr<yw::Instrument> { ++instruments; return std::make_shared<Silent>(); };
    f.pluginEffects = [&] (const yw::PluginRef&, double) -> std::shared_ptr<yw::Effect> { ++effects; return std::make_shared<Pass>(); };
    const std::function<void (yw::PluginRef&)> changes[] = {
        [] (yw::PluginRef& r) { r.releaseVelocity = ! r.releaseVelocity; },
        [] (yw::PluginRef& r) { r.sendBendRange = ! r.sendBendRange; },
        [] (yw::PluginRef& r) { r.bendRange = 24; },
        [] (yw::PluginRef& r) { r.modXAftertouch = ! r.modXAftertouch; },
        [] (yw::PluginRef& r) { r.allNotesOffOnStop = ! r.allNotesOffOnStop; },
        [] (yw::PluginRef& r) { r.resetOnFlush = ! r.resetOnFlush; },
        [] (yw::PluginRef& r) { r.sendLoopPosition = ! r.sendLoopPosition; },
    };
    auto s = song();
    auto state = yw::RackState::build (s, nullptr, 48000.0, f);
    CHECK (instruments == 1 && effects == 1);
    // nothing changed: the running plugins stay
    state = yw::RackState::build (s, state.get(), 48000.0, f);
    CHECK (instruments == 1 && effects == 1);
    int wantedInstruments = 1, wantedEffects = 1;
    for (const auto& change : changes)
    {
        change (s.channels[0].plugin);
        state = yw::RackState::build (s, state.get(), 48000.0, f);
        CHECK (instruments == ++wantedInstruments && effects == wantedEffects);
        change (s.mixer[1].effects[0].plugin);
        state = yw::RackState::build (s, state.get(), 48000.0, f);
        CHECK (instruments == wantedInstruments && effects == ++wantedEffects);
    }
    // an option read while the plugin runs doesn't cost a re-creation
    s.channels[0].plugin.notifyRender = ! s.channels[0].plugin.notifyRender;
    state = yw::RackState::build (s, state.get(), 48000.0, f);
    CHECK (instruments == wantedInstruments && effects == wantedEffects);
}
