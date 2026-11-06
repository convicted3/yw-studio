cmake_minimum_required(VERSION 3.22)

project(YWStudio VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static MSVC runtime: the .exe runs without installing the VC++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# JUCE (AGPLv3 for open-source use). Pass -DFETCHCONTENT_SOURCE_DIR_JUCE=<path>
# to build against a local checkout instead of downloading.
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_gui_app(YWStudio
    PRODUCT_NAME          "YW Studio"
    COMPANY_NAME          "YW"
    COMPANY_WEBSITE       "https://yolo.su"
    BUNDLE_ID             "su.yolo.ywstudio"
    VERSION               ${PROJECT_VERSION}
    ICON_BIG              "${CMAKE_CURRENT_SOURCE_DIR}/resources/yw_icon.png"
    ICON_SMALL            "${CMAKE_CURRENT_SOURCE_DIR}/resources/yw_icon.png")

juce_generate_juce_header(YWStudio)

target_sources(YWStudio PRIVATE
    src/Main.cpp
    src/AppState.cpp
    src/LookAndFeel.cpp
    src/SplashWindow.cpp
    src/MainComponent.cpp
    src/MenuModel.cpp
    src/Icons.cpp
    src/Widgets.cpp
    src/TopBar.cpp
    src/BrowserPanel.cpp
    src/ChannelRackView.cpp
    src/MixerView.cpp
    src/PlaylistView.cpp
    src/PanelWindow.cpp
    src/PluginManagerWindow.cpp
    src/SettingsWindow.cpp)

target_include_directories(YWStudio PRIVATE src)

juce_add_binary_data(YWStudioData SOURCES
    resources/yw_logo.png
    resources/yw_icon.png
    resources/fonts/BarlowSemiCondensed-Regular.ttf
    resources/fonts/BarlowSemiCondensed-Medium.ttf
    resources/fonts/BarlowSemiCondensed-SemiBold.ttf)

target_compile_definitions(YWStudio PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:YWStudio,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:YWStudio,JUCE_VERSION>"
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_LADSPA=0
    JUCE_PLUGINHOST_LV2=0)

target_link_libraries(YWStudio
    PRIVATE
        YWStudioData
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
