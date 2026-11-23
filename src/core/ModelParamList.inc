// Automatable engine parameters (core/ModelParams.h): one line per engine, YW_MODEL_PARAMS (<InstrumentKind name>,
// <function returning modelparams::Table>), the function declared in core/ModelParams.h or in the engine's header.
YW_MODEL_PARAMS (toxic, toxicModelTable)       // round 168n
YW_MODEL_PARAMS (poizone, poizoneModelTable)   // round 168n
YW_MODEL_PARAMS (fruitkick, fruitKickModelTable)   // round 168p (core/DrumControls.cpp)
YW_MODEL_PARAMS (plucked, pluckedModelTable)
YW_MODEL_PARAMS (drumpad, drumpadModelTable)
YW_MODEL_PARAMS (drumaxx, drumaxxModelTable)
YW_MODEL_PARAMS (bassdrum, bassDrumModelTable)
YW_MODEL_PARAMS (drumsynth, drumSynthModelTable)
YW_MODEL_PARAMS (sakura, sakuraModelTable)
YW_MODEL_PARAMS (slayer, slayerModelTable)
YW_MODEL_PARAMS (harmless, harmlessModelTable)     // round 168o (core/ModelParamsSynths.h)
YW_MODEL_PARAMS (dx10, dx10ModelTable)             // round 168o
YW_MODEL_PARAMS (simsynth, simSynthModelTable)     // round 168o
YW_MODEL_PARAMS (ogun, ogunModelTable)             // round 168o
YW_MODEL_PARAMS (autogun, autogunModelTable)       // round 168o
YW_MODEL_PARAMS (minisynth, miniSynthModelTable)   // round 168o
YW_MODEL_PARAMS (gms, gmsModelTable)               // round 168o
YW_MODEL_PARAMS (morphine, morphineModelTable)     // round 168o
YW_MODEL_PARAMS (kepler, keplerModelTable)                 // round 168q
YW_MODEL_PARAMS (keplerExo, keplerExoModelTable)           // round 168q
YW_MODEL_PARAMS (transistorbass, transistorBassModelTable) // round 168q
YW_MODEL_PARAMS (sawer, sawerModelTable)                   // round 168q
YW_MODEL_PARAMS (beepmap, beepMapModelTable)               // round 168q
YW_MODEL_PARAMS (granulizer, granulizerModelTable)         // round 168q
YW_MODEL_PARAMS (pocketSuperSaw, pocketSuperSawModelTable)   // round 190by (core/PocketSuperSaw.h)
YW_MODEL_PARAMS (tetra, tetraModelTable)                     // round 190hs (core/FmSynth.h)
YW_MODEL_PARAMS (ywModel, ywModelModelTable)             // round 190ni (core/physmod/YwModel.h)
YW_MODEL_PARAMS (helix, helixModelTable)                     // round 190ja (core/WavetableSynth.h)
YW_MODEL_PARAMS (braid, braidModelTable)                     // round 190ko (core/Braid.h)
