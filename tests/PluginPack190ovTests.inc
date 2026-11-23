// Round 190ov (0.1.31): YW's later own engines in the plugin pack for other DAWs (docs/PLUGINS-PACK.md)

TEST (pluginpack190ov_later_own_engines_are_in_the_pack)
{
    using namespace yw;
    // round 190ov (Discord: YW's own effects and instruments for other DAWs): the later YW-own engines are in the CLAP,
    // YW Model also as VST3 / LV2; the engines whose export names wait for the owner are not
    std::map<std::string, const plugexport::Entry*> byName;
    for (const auto& e : plugexport::catalog)
        byName[e.name] = &e;
    const std::pair<const char*, EffectKind> effects[] = { { "YW Strike Shaper", EffectKind::strikeShaper }, { "YW Anvil Bus", EffectKind::anvilBus },
                                                           { "YW Corrode", EffectKind::corrode }, { "YW Soundboard", EffectKind::soundboard },
                                                           { "YW Platter", EffectKind::platter }, { "YW Timeprism", EffectKind::timeprism },
                                                           { "YW Chimefield", EffectKind::harmonia } }; // (rollup 0.1.31 step 6: Harmonia exports as YW Chimefield)
    for (const auto& [name, kind] : effects)
        CHECK (byName.count (name) == 1 && ! byName[name]->instrument && byName[name]->effect == kind);
    CHECK (byName.count ("YW Model") == 1 && byName["YW Model"]->instrument && byName["YW Model"]->synth == InstrumentKind::ywModel);
    // rollup 0.1.31 step 6 (owner decision): Tetra, Helix and Braid under export names of their own; in the CLAP, as VST3 and
    // on Linux as LV2 since round 190pq (their state saves back byte for byte); the engines' own names (other makers'
    // products) are never an export name
    const std::pair<const char*, InstrumentKind> synths[] = { { "YW Fourfold", InstrumentKind::tetra }, { "YW Wavecoil", InstrumentKind::helix },
                                                              { "YW Duoloom", InstrumentKind::braid } };
    for (const auto& [name, kind] : synths)
        CHECK (byName.count (name) == 1 && byName[name]->instrument && byName[name]->synth == kind);
    for (const auto& e : plugexport::catalog)
        for (const char* taken : { "Tetra", "Helix", "Braid", "Harmonia" })
            CHECK (std::string (e.name).find (taken) == std::string::npos);
    // the clap ids keep the "su.yolo.yw." prefix the LV2 URIs are made from (cmake/YwPlugins.cmake)
    for (const auto& e : plugexport::catalog)
        CHECK (std::string (e.clapId).rfind ("su.yolo.yw.", 0) == 0 && std::string (e.clapId).size() > 15);
}
