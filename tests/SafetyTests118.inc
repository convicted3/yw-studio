// --- Project safety: backups, "save new version", crash recovery, packages (core/ProjectSafety.h) - REA round 118 ---

TEST (safety_backup_names_like_fl)
{
    using namespace yw::safety;
    const auto a = backupName ("Night Drive", BackupKind::autosaved, 2026, 10, 20, 14, 9);
    CHECK (a == "Night Drive (autosaved on 2026-10-20 at 14h09)");
    const auto o = backupName ("Night Drive", BackupKind::overwritten, 2026, 1, 2, 3, 4);
    CHECK (o == "Night Drive (overwritten on 2026-01-02 at 03h04)");
    CHECK (isBackupOf (a, "Night Drive") && isBackupOf (o, "Night Drive") && isBackupOf (a + "_3", "Night Drive"));
    CHECK (isBackupOf ("Night Drive (2026-10-16 09.05.03)", "Night Drive")); // older YW names
    CHECK (isBackupOf ("Night Drive", "Night Drive"));
    CHECK (! isBackupOf ("Night Drive 2 (autosaved on 2026-10-20 at 14h09)", "Night Drive"));
    CHECK (! isBackupOf ("Night Drive (autosaved on 2026-10-20 at 14h0x)", "Night Drive"));
    CHECK (! isBackupOf ("Night Drive (saved on 2026-10-20 at 14h09)", "Night Drive"));
    CHECK (backupKind (o) == BackupKind::overwritten && backupKind (a) == BackupKind::autosaved);
    CHECK (backupKind ("Song (2026-10-16 09.05.03)") == BackupKind::autosaved);
}

TEST (safety_backup_rotation_by_time)
{
    using namespace yw::safety;
    const std::vector<BackupFile> files { { "Song (autosaved on 2026-10-20 at 10h00)", 100 },
                                          { "Song (overwritten on 2026-10-20 at 09h00)", 50 },
                                          { "Song (2026-10-19 23.00.00)", 10 },
                                          { "Song", 5 },
                                          { "Other (autosaved on 2026-10-20 at 11h00)", 200 },
                                          { "Song (autosaved on 2026-10-20 at 10h00)_2", 150 } };
    const auto gone = backupsToRemove (files, "Song", 2);
    CHECK (gone.size() == 3);
    CHECK (gone.size() == 3 && gone[0] == "Song" && gone[1] == "Song (2026-10-19 23.00.00)" && gone[2] == "Song (overwritten on 2026-10-20 at 09h00)");
    CHECK (backupsToRemove (files, "Song", 10).empty() && backupsToRemove (files, "Other", 1).empty());
    CHECK (newestBackup (files, "Song") == "Song (autosaved on 2026-10-20 at 10h00)_2");
    CHECK (newestBackup (files, "Nope").empty());

    const std::vector<BackupFile> ow { { "A (overwritten on 2026-10-20 at 09h00)", 90 }, { "A (autosaved on 2026-10-20 at 08h00)", 80 } };
    CHECK (newestBackup (ow, "A") == "A (overwritten on 2026-10-20 at 09h00)");
    CHECK (newestBackup (ow, "A", false) == "A (autosaved on 2026-10-20 at 08h00)");
}

TEST (safety_crash_recovery_candidate)
{
    using namespace yw::safety;
    const std::vector<BackupFile> files { { "Beat (autosaved on 2026-10-20 at 10h00)", 100 },
                                          { "recovered 2026-10-20 10-05-00", 105 },
                                          { "Beat (overwritten on 2026-10-20 at 10h06)", 106 },
                                          { "Old (autosaved on 2026-10-19 at 10h00)", 10 } };
    CHECK (crashRecoveryCandidate (files, 50) == "recovered 2026-10-20 10-05-00"); // the overwritten copy is older content
    CHECK (crashRecoveryCandidate (files, 101) == "recovered 2026-10-20 10-05-00");
    CHECK (crashRecoveryCandidate (files, 200).empty()); // nothing written in that session
}

TEST (safety_unique_and_version_names)
{
    using namespace yw::safety;
    std::set<std::string> taken { "Song", "Song_2", "Beat_7", "Beat_8" };
    auto isTaken = [&] (const std::string& s) { return taken.count (s) > 0; };
    CHECK (uniqueName ("Free", isTaken) == "Free");
    CHECK (uniqueName ("Song", isTaken) == "Song_3");
    CHECK (nextVersionName ("Song", isTaken) == "Song_3");   // Song_2 exists
    CHECK (nextVersionName ("Song_2", isTaken) == "Song_3");
    CHECK (nextVersionName ("Beat_7", isTaken) == "Beat_9");
    CHECK (nextVersionName ("Mix", {}) == "Mix_2");
    CHECK (nextVersionName ("Mix_v2", {}) == "Mix_v2_2");    // "_v2" is not a version number
    CHECK (nextVersionName ("_5", {}) == "_5_2");            // nothing before the underscore
}

TEST (safety_paths_and_package_plan)
{
    using namespace yw::safety;
    CHECK (isRelativePath ("samples/kick.wav") && ! isRelativePath ("C:\\x\\kick.wav") && ! isRelativePath ("/home/a.wav")
           && ! isRelativePath ("\\\\server\\a.wav") && ! isRelativePath (""));
    CHECK (joinPath ("C:\\Projects\\Beat", "samples/kick.wav") == "C:\\Projects\\Beat\\samples\\kick.wav");
    CHECK (joinPath ("/home/u/Beat/", "samples/kick.wav") == "/home/u/Beat/samples/kick.wav");

    yw::Song song;
    auto add = [&] (yw::InstrumentKind kind, const std::string& path) {
        yw::ChannelDef c;
        c.instrument = kind;
        c.samplePath = path;
        song.channels.push_back (c);
    };
    add (yw::InstrumentKind::sampler, "D:\\Drums\\Kick.wav");
    add (yw::InstrumentKind::sampler, "E:\\Other\\kick.wav"); // same file name, another file
    add (yw::InstrumentKind::audioClip, "D:\\Vox\\lead.flac");
    add (yw::InstrumentKind::sampler, "D:\\Drums\\Kick.wav"); // used twice: one entry
    add (yw::InstrumentKind::synth, "D:\\ignored.wav");       // not a sample user
    CHECK (referencedSamples (song).size() == 3);

    const auto plan = packagePlan (song);
    CHECK (plan.size() == 3);
    CHECK (plan.at ("D:\\Drums\\Kick.wav") == "samples/Kick.wav");
    CHECK (plan.at ("E:\\Other\\kick.wav") == "samples/kick_2.wav");
    CHECK (plan.at ("D:\\Vox\\lead.flac") == "samples/lead.flac");

    // a package opened in /x/Beat: relative paths go inside it; a moved sample is found next to the project
    yw::applyRelocations (song, plan);
    song.channels[2].samplePath = "C:\\Old place\\lead.flac";
    std::set<std::string> disk { "/x/Beat/samples/Kick.wav", "/x/Beat/Audio/lead.flac" };
    const auto changed = resolveProjectSamples (song, "/x/Beat", [&] (const std::string& p) { return disk.count (p) > 0; });
    CHECK (changed == 4);
    CHECK (song.channels[0].samplePath == "/x/Beat/samples/Kick.wav" && song.channels[3].samplePath == "/x/Beat/samples/Kick.wav");
    CHECK (song.channels[1].samplePath == "/x/Beat/samples/kick_2.wav"); // not on disk: still a full path for the Missing files dialog
    CHECK (song.channels[2].samplePath == "/x/Beat/Audio/lead.flac");
    CHECK (resolveProjectSamples (song, "/x/Beat", [&] (const std::string& p) { return disk.count (p) > 0; }) == 0);
}

// D#92, round 180a: a dotted name keeps its dot; the string macOS hands to a running application
TEST (safety_project_extension_and_open_requests)
{
    using namespace yw::safety;
    CHECK (withProjectExtension ("beat", ".yw") == "beat.yw");
    CHECK (withProjectExtension ("beat.v2", ".yw") == "beat.v2.yw");
    CHECK (withProjectExtension ("beat.yw", ".yw") == "beat.yw");
    CHECK (withProjectExtension ("BEAT.YW", ".yw") == "BEAT.YW");
    CHECK (withProjectExtension ("yw", ".yw") == "yw.yw");

    CHECK (isFileArgument ("beat.yw") && isFileArgument ("/x/beat.yw") && isFileArgument ("C:\\x\\beat.yw"));
    CHECK (! isFileArgument ("") && ! isFileArgument ("--lang") && ! isFileArgument ("-psn_0_123"));

    using Files = std::vector<std::string>;
    CHECK (filesInCommandLine ("") == Files {});
    CHECK (filesInCommandLine ("/x/beat.yw") == Files { "/x/beat.yw" });
    CHECK (filesInCommandLine ("\"/My Beats/a b.yw\"") == Files { "/My Beats/a b.yw" });
    CHECK (filesInCommandLine ("\"/a b/c.yw\" /d.yw") == (Files { "/a b/c.yw", "/d.yw" }));
    CHECK (filesInCommandLine ("/say\\\"hi\\\".yw") == Files { "/say\"hi\".yw" });
    CHECK (filesInCommandLine ("-psn_0_123 /x/m\xc3\xbasica.yw") == Files { "/x/m\xc3\xbasica.yw" });
}
