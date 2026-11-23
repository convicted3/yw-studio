# YW's own plugins as VST3 / CLAP (round 180b, docs/PLUGIN_EXPORT.md; LV2 on Linux behind YW_PLUGINS_LV2, round 190ov). Included from CMakeLists.txt only with
# -DYW_BUILD_PLUGINS=ON. One static library holds JUCE and the app's sources (compiled once); each VST3 adds JUCE's wrapper
# for its catalog entry, and one CLAP module holds every entry.

add_library(yw_plugin_runtime STATIC src/plugin/ExportProcessor.cpp ${YW_APP_SOURCES})
set(YW_PLUGIN_DEFINES
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_PLUGINHOST_VST3=1 JUCE_USE_MP3AUDIOFORMAT=0
    JUCE_STANDALONE_APPLICATION=0 JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
    JUCE_APPLICATION_NAME_STRING="YW" JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}"
    YW_EXPORT_VERSION="${PROJECT_VERSION}")
foreach(m juce_core juce_data_structures juce_events juce_graphics juce_gui_basics juce_gui_extra juce_audio_basics
          juce_audio_devices juce_audio_formats juce_audio_processors juce_audio_utils juce_dsp juce_opengl)
    list(APPEND YW_PLUGIN_DEFINES JUCE_MODULE_AVAILABLE_${m}=1)
endforeach()
if(WIN32)
    list(APPEND YW_PLUGIN_DEFINES JUCE_ASIO=1)
    target_include_directories(yw_plugin_runtime PRIVATE third_party/asiosdk/common)
    target_link_libraries(yw_plugin_runtime PRIVATE winhttp ws2_32 iphlpapi) # (iphlpapi, ws2_32: Ableton Link, round 190gl)
    target_link_libraries(yw_plugin_runtime PRIVATE mfplat mfreadwrite mfuuid) # (Media Foundation: video decoding, M4A export)
elseif(APPLE)
    list(APPEND YW_PLUGIN_DEFINES JUCE_PLUGINHOST_AU=1)
    # the system's video decoder, as in the app (ui/VideoMediaMac.mm; manual retain / release)
    target_sources(yw_plugin_runtime PRIVATE src/ui/VideoMediaMac.mm)
    set_source_files_properties(src/ui/VideoMediaMac.mm PROPERTIES COMPILE_OPTIONS "-fno-objc-arc")
    target_link_libraries(yw_plugin_runtime PRIVATE "-framework AVFoundation" "-framework CoreMedia" "-framework CoreVideo"
                          "-framework AudioToolbox" "-framework CoreFoundation" ${CMAKE_DL_LIBS}) # (AudioToolbox: M4A export)
    # the moving marks' backend, as in the app: ui/MotionOverlay.cpp (in YW_APP_SOURCES) calls it (round 190nk)
    target_sources(yw_plugin_runtime PRIVATE src/ui/MotionOverlayMac.mm)
    set_source_files_properties(src/ui/MotionOverlayMac.mm PROPERTIES COMPILE_OPTIONS "-fno-objc-arc")
    target_link_libraries(yw_plugin_runtime PRIVATE "-framework QuartzCore")
else()
    list(APPEND YW_PLUGIN_DEFINES JUCE_ALSA=1 JUCE_JACK=1 JUCE_PLUGINHOST_LV2=1)
    target_include_directories(yw_plugin_runtime SYSTEM PRIVATE third_party/pipewire/include/pipewire-0.3 third_party/pipewire/include/spa-0.2)
    target_link_libraries(yw_plugin_runtime PRIVATE ${CMAKE_DL_LIBS})
    # the moving marks' X11 backend, as in the app: ui/MotionOverlay.cpp (in YW_APP_SOURCES) calls it (round 190nk; the
    # 0.1.30 release run's Linux plugin link failed without it)
    target_sources(yw_plugin_runtime PRIVATE src/ui/MotionOverlayX11.cpp)
endif()
target_compile_definitions(yw_plugin_runtime PUBLIC ${YW_PLUGIN_DEFINES})
target_include_directories(yw_plugin_runtime
    PUBLIC src third_party/clap/include "${juce_SOURCE_DIR}/modules" "${juce_SOURCE_DIR}/modules/juce_audio_processors/format_types/VST3_SDK"
    PRIVATE third_party/onnxruntime ${CMAKE_CURRENT_BINARY_DIR}/yw_build)
target_link_libraries(yw_plugin_runtime
    PUBLIC yw_core
    PRIVATE juce::juce_audio_utils juce::juce_audio_processors juce::juce_dsp juce::juce_opengl)
set_target_properties(yw_plugin_runtime PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)

# the host's symbols are never taken for ours, and only the entry points are visible
function(yw_finish_plugin target)
    set_target_properties(${target} PROPERTIES PREFIX "" CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
    target_link_libraries(${target} PRIVATE yw_plugin_runtime)
    if(WIN32)
        target_sources(${target} PRIVATE resources/YwPlugin.rc)
    endif()
    if(APPLE)
        target_link_options(${target} PRIVATE -Wl,-dead_strip)
    endif()
    if(UNIX AND NOT APPLE)
        target_link_options(${target} PRIVATE -static-libstdc++ -static-libgcc -Wl,--gc-sections -Wl,--exclude-libs,ALL -Wl,-z,defs)
        target_compile_options(${target} PRIVATE -ffunction-sections -fdata-sections)
    endif()
endfunction()

if(WIN32)
    set(YW_VST3_ARCH x86_64-win)
elseif(APPLE)
    set(YW_VST3_ARCH MacOS)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
    set(YW_VST3_ARCH aarch64-linux)
else()
    set(YW_VST3_ARCH x86_64-linux)
endif()

set(YW_PLUGIN_OUT "${CMAKE_BINARY_DIR}/plugins")
# round 190ov: the VST3 short list also as LV2 bundles on Linux (JUCE's LV2 wrapper, plugin/Lv2Entry.cpp). On since round
# 190pk (0.1.31), which fixed what made JUCE's LV2 host fail on them (cmake/YwPatchJuce.cmake, fix 50)
option(YW_PLUGINS_LV2 "Linux: build the VST3 short list as LV2 bundles too" ON)
# every built entry of the one table, src/plugin/plugins.def; the index counts the built entries (plugin/ExportCatalog.h).
# built = 2: a VST3 of its own as well (built = 1: in the CLAP only)
file(STRINGS src/plugin/plugins.def YW_PLUGIN_LINES REGEX "^YW_PLUGIN .*, [12][)]$")
# (round 190pq: an edit of the table re-runs CMake, so a newly built entry gets its targets in an existing build folder)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/plugin/plugins.def")
set(index 0)
set(YW_PLUGIN_TARGETS yw_clap)
foreach(line IN LISTS YW_PLUGIN_LINES)
    # (round 190ov: the LV2 URI comes from the clap id, read first: a REGEX REPLACE resets CMAKE_MATCH_<n>)
    string(REGEX REPLACE "^YW_PLUGIN [(]\"[^\"]+\", [01], [^,]+, [^,]+, \"com[.]ka1-s[.]yw[.]([^\"]+)\", .*$" "\\1" lv2_id "${line}")
    if(NOT line MATCHES "^YW_PLUGIN [(]\"([^\"]+)\", ([01]), [^,]+, [^,]+, \"[^\"]+\", (0x[0-9a-f]+), \"([^\"]+)\", ([12])[)]$")
        message(FATAL_ERROR "src/plugin/plugins.def: can't read: ${line}")
    endif()
    if(CMAKE_MATCH_5 STREQUAL "2")
        set(name "${CMAKE_MATCH_1}")
        set(target "yw_vst3_${index}")
        add_library(${target} MODULE src/plugin/Vst3Entry.cpp)
        target_compile_definitions(${target} PRIVATE YW_EXPORT_INDEX=${index} JucePlugin_Name="${name}" JucePlugin_IsSynth=${CMAKE_MATCH_2}
            JucePlugin_PluginCode=${CMAKE_MATCH_3} JucePlugin_Vst3Category="${CMAKE_MATCH_4}")
        yw_finish_plugin(${target})
        if(APPLE)
            # a bundle: <name>.vst3/Contents/{Info.plist, MacOS/<name>}
            set_source_files_properties(src/plugin/Vst3Entry.cpp PROPERTIES COMPILE_OPTIONS "-xobjective-c++")
            string(REGEX REPLACE "[^A-Za-z0-9]" "-" bundle_id "${name}")
            set_target_properties(${target} PROPERTIES BUNDLE TRUE BUNDLE_EXTENSION vst3 OUTPUT_NAME "${name}"
                LIBRARY_OUTPUT_DIRECTORY "${YW_PLUGIN_OUT}"
                MACOSX_BUNDLE_GUI_IDENTIFIER "su.yolo.yw.vst3.${bundle_id}"
                MACOSX_BUNDLE_BUNDLE_NAME "${name}"
                MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
                MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
                MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/resources/plugin/Info.plist.in")
        else()
            set_target_properties(${target} PROPERTIES
                OUTPUT_NAME "${name}"
                LIBRARY_OUTPUT_DIRECTORY "${YW_PLUGIN_OUT}/${name}.vst3/Contents/${YW_VST3_ARCH}")
        endif()
        if(WIN32)
            set_target_properties(${target} PROPERTIES SUFFIX ".vst3")
        endif()
        list(APPEND YW_PLUGIN_TARGETS ${target})

        # round 190ov: on Linux the same short list as LV2 bundles (JUCE's LV2 wrapper, plugin/Lv2Entry.cpp):
        # <name with dashes>.lv2/{<name>.so, manifest.ttl, dsp.ttl, ui.ttl}, the .ttl files written by JUCE's juce_lv2_helper
        if(UNIX AND NOT APPLE AND YW_PLUGINS_LV2)
            set(lv2_defs YW_EXPORT_INDEX=${index} JucePlugin_Name="${name}" JucePlugin_IsSynth=${CMAKE_MATCH_2}
                JucePlugin_PluginCode=${CMAKE_MATCH_3} JucePlugin_Vst3Category="${CMAKE_MATCH_4}"
                JucePlugin_LV2URI="https://yolo.su/plugins/lv2/${lv2_id}")
            # (round 190pk: an effect's LV2 class from its VST3 category, so LV2 hosts file it; JUCE wrote lv2:Plugin)
            set(lv2_class "")
            set(lv2_category "${CMAKE_MATCH_4}") # (copied: the MATCHES below resets CMAKE_MATCH_<n>)
            if(CMAKE_MATCH_2 STREQUAL "0")
                foreach(pair "EQ=EQPlugin" "Dynamics=DynamicsPlugin" "Reverb=ReverbPlugin" "Delay=DelayPlugin" "Pitch=PitchPlugin")
                    string(REPLACE "=" ";" pair "${pair}")
                    list(GET pair 0 word)
                    list(GET pair 1 cls)
                    if(lv2_class STREQUAL "" AND lv2_category MATCHES "(^|[|])${word}")
                        set(lv2_class "lv2:${cls}")
                    endif()
                endforeach()
            endif()
            if(NOT lv2_class STREQUAL "")
                list(APPEND lv2_defs JucePlugin_LV2Class="${lv2_class}")
            endif()
            string(REGEX REPLACE "[^A-Za-z0-9]" "-" lv2_name "${name}")
            set(lv2_target "yw_lv2_${index}")
            add_library(${lv2_target} MODULE src/plugin/Lv2Entry.cpp)
            target_compile_definitions(${lv2_target} PRIVATE ${lv2_defs})
            target_include_directories(${lv2_target} PRIVATE src/plugin/lv2)
            target_link_libraries(${lv2_target} PRIVATE juce_lilv_headers)
            yw_finish_plugin(${lv2_target})
            set_target_properties(${lv2_target} PROPERTIES OUTPUT_NAME "${lv2_name}" LIBRARY_OUTPUT_DIRECTORY "${YW_PLUGIN_OUT}/${lv2_name}.lv2")
            if(NOT TARGET yw_lv2_helper)
                add_executable(yw_lv2_helper "${juce_SOURCE_DIR}/modules/juce_audio_plugin_client/LV2/juce_LV2ManifestHelper.cpp")
                target_compile_features(yw_lv2_helper PRIVATE cxx_std_17)
                target_link_libraries(yw_lv2_helper PRIVATE ${CMAKE_DL_LIBS})
            endif()
            add_custom_command(TARGET ${lv2_target} POST_BUILD COMMAND yw_lv2_helper "$<TARGET_FILE:${lv2_target}>" VERBATIM)
            list(APPEND YW_PLUGIN_TARGETS ${lv2_target})
        endif()
    endif()
    math(EXPR index "${index} + 1")
endforeach()

add_library(yw_clap MODULE src/plugin/ClapEntry.cpp)
yw_finish_plugin(yw_clap)
if(APPLE)
    # a bundle: YW Plugins.clap/Contents/{Info.plist, MacOS/YW Plugins}
    set_target_properties(yw_clap PROPERTIES BUNDLE TRUE BUNDLE_EXTENSION clap OUTPUT_NAME "YW Plugins"
        LIBRARY_OUTPUT_DIRECTORY "${YW_PLUGIN_OUT}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "su.yolo.yw.clap"
        MACOSX_BUNDLE_BUNDLE_NAME "YW Plugins"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/resources/plugin/Info.plist.in")
else()
    set_target_properties(yw_clap PROPERTIES OUTPUT_NAME "YW Plugins" SUFFIX ".clap" LIBRARY_OUTPUT_DIRECTORY "${YW_PLUGIN_OUT}")
endif()

# the check: loads the built plugins as a host does (docs/PLUGIN_EXPORT.md)
add_executable(plugexporttest tools/plugexporttest.cpp)
target_link_libraries(plugexporttest PRIVATE yw_plugin_runtime)

# every plugin of the table: cmake --build <dir> --target yw_plugins (scripts/package-plugins.sh packs them)
add_custom_target(yw_plugins DEPENDS ${YW_PLUGIN_TARGETS} plugexporttest)
