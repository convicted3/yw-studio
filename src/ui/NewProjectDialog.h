#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/Song.h"
#include "core/Templates.h"

namespace yw
{
// The New project window (FL's "New project", interpreted; REA_rodada84): a template (YW's default, the built-in
// ones, or the user's own .yw files in Documents\YW\Templates), name and folder, tempo and time signature, the
// project's credits. `onCreate` receives the new song and, when a folder was chosen, the file it should be saved to.
using NewProjectCallback = std::function<void (Song song, juce::File file)>;
void showNewProjectDialog (NewProjectCallback onCreate);

// Whether File › New opens the window (the "Don't show this again" choice), and the user templates' folder.
bool newProjectDialogEnabled();
juce::File userTemplatesFolder();

// The default template (issue #123; FL's Options › General › "Default template"): what YW opens at start-up and what
// File › New starts from, always as a new untitled project. Kept in new-project.txt ("defaultTemplate=").
TemplateChoice defaultTemplate();
void setDefaultTemplate (const TemplateChoice& choice);
// FL's "Startup project" (Settings › General › Startup): an empty project, the default template or the last project.
StartupProject startupProject();
void setStartupProject (StartupProject choice);
// Every template that can be the default: YW's default song, the built-in ones, then the user's (by name).
std::vector<TemplateChoice> availableTemplates();
// The name shown for a choice (a built-in one translated, a user one without its extension).
juce::String templateChoiceName (const TemplateChoice& choice);
// A new song from a template (YW's own follow Options › Mixer; a user's stays as saved); nullopt when a user
// template's file is gone or unreadable.
std::optional<Song> makeFromTemplate (const TemplateChoice& choice);
// A new song from the default template; YW's default song when that template is gone (`missing` then true).
Song makeDefaultTemplateProject (bool* missing = nullptr);

// Saves `song` as a user template named `name` (File › Save as template...). False with `error` on failure.
bool saveUserTemplate (const Song& song, const juce::String& name, juce::String& error);

// The window's content, for --snapshot mode (sample user templates, nothing saved).
std::unique_ptr<juce::Component> makeNewProjectPanel();
} // namespace yw
