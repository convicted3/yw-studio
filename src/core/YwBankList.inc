// YW's own preset banks (core/YwBanks.h): one line per engine, YW_BANK (<InstrumentKind name>, <factory>).
// The factory is defined in yw::banks by the engine's bank file (add that file to CMakeLists.txt,
// scripts/build-local.sh CORE and scripts/build-windows-zig.sh next to src/core/YwBanks.cpp).
YW_BANK (toxic, toxicBank)       // core/ToxicBank.cpp (round 168n)
YW_BANK (poizone, poizoneBank)   // core/PoiZoneBank.cpp (round 168n)
YW_BANK (fruitkick, fruitKickBank)   // core/DrumBanks.cpp (round 168p)
YW_BANK (plucked, pluckedBank)       // core/DrumBanks.cpp (round 168p)
YW_BANK (drumpad, drumpadBank)       // core/DrumBanks.cpp (round 168p)
YW_BANK (sakura, sakuraBank)         // core/DrumBanks.cpp (round 168p)
YW_BANK (drumaxx, drumaxxBank)       // core/DrumBanks.cpp (round 168p)
YW_BANK (drumsynth, drumSynthBank)   // core/DrumBanks.cpp (round 168p)
YW_BANK (harmless, harmlessBank) // core/HarmlessBank.cpp (round 168o)
YW_BANK (dx10, dx10Bank)         // core/Dx10Bank.cpp (round 168o)
YW_BANK (simsynth, simSynthBank) // core/SimSynthBank.cpp (round 168o)
YW_BANK (beepmap, beepMapBank)       // core/BeepMapBank.cpp (round 168q)
YW_BANK (granulizer, granulizerBank) // core/GranulizerBank.cpp (round 168q)
YW_BANK (helix, helixBank)         // core/HelixBank.cpp (round 190mi)
YW_BANK (ywModel, ywModelBank) // core/YwModelBank.cpp (round 190ni)
