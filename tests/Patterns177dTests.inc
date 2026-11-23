// Round 177d, FL pain points (research-web-2.md §3; docs/analise/REA_rodada177d.md §2).
// Slice 177d-5: 31a pattern cleanup (delete unused, sort, merge identical).

namespace pain177d
{
yw::Song threePatterns()
{
    auto s = yw::Song::makeDefault();
    s.patterns.clear();
    s.playlist.clear();
    s.arrangements.clear();
    s.currentArrangement = 0;
    for (const auto* name : { "Bass", "drums", "Arp" })
    {
        yw::Pattern p;
        p.name = name;
        yw::Note n;
        n.key = 60;
        n.length = 240;
        p.notes.push_back (n);
        s.patterns.push_back (p);
    }
    s.currentPattern = 2;
    return s;
}
yw::Clip clipOf (int pattern, std::int64_t start)
{
    yw::Clip c;
    c.kind = yw::ClipKind::pattern;
    c.pattern = pattern;
    c.start = start;
    c.length = 960;
    return c;
}
} // namespace pain177d

TEST (pain177d_unused_patterns_and_delete)
{
    auto s = pain177d::threePatterns();
    s.playlist.push_back (pain177d::clipOf (1, 0));
    CHECK ((yw::unusedPatterns (s) == std::vector<int> { 0, 2 }));
    // a pattern block counts as a use
    s.blocks.push_back ({ 2, 0 });
    CHECK ((yw::unusedPatterns (s) == std::vector<int> { 0 }));
    CHECK (yw::deletePatterns (s, { 0 }) == 1);
    CHECK (s.patterns.size() == 2 && s.patterns[0].name == "drums" && s.playlist[0].pattern == 0 && s.blocks[0].pattern == 1);
    CHECK (s.currentPattern == 1); // (it was "Arp", which moved up)
    // a pattern a Loop Starter channel belongs to is in use
    auto ls = pain177d::threePatterns();
    ls.channels.resize (1);
    ls.channels[0].loopStarterPattern = 1;
    CHECK ((yw::unusedPatterns (ls) == std::vector<int> { 0, 2 }));
    // the last pattern is never deleted
    auto one = pain177d::threePatterns();
    CHECK (yw::deletePatterns (one, { 0, 1, 2 }) == 2 && one.patterns.size() == 1);
}

TEST (pain177d_sort_patterns_keeps_clips_on_their_pattern)
{
    auto s = pain177d::threePatterns();
    s.playlist.push_back (pain177d::clipOf (0, 0));   // Bass
    s.playlist.push_back (pain177d::clipOf (2, 960)); // Arp
    yw::sortPatterns (s, yw::PatternSort::name);  // Arp, Bass, drums (case ignored)
    CHECK (s.patterns[0].name == "Arp" && s.patterns[1].name == "Bass" && s.patterns[2].name == "drums");
    CHECK (s.playlist[0].pattern == 1 && s.playlist[1].pattern == 0);
    CHECK (s.currentPattern == 0); // the current one was Arp

    s.patterns[0].colour = 0xff0000ff; // Arp blue
    s.patterns[1].colour = 0xffff0000; // Bass red
    yw::sortPatterns (s, yw::PatternSort::colour); // red, blue, then the uncoloured
    CHECK (s.patterns[0].name == "Bass" && s.patterns[1].name == "Arp" && s.patterns[2].name == "drums");
    CHECK (s.playlist[0].pattern == 0 && s.playlist[1].pattern == 1);
}

TEST (pain177d_sort_leaves_hand_sized_clips_alone)
{
    // why the sort goes through AudioEngine::editSongPatternOrder and not editSong: followPatternLengths compares by
    // index, so after a sort it would take a pattern's move for a change of length
    auto s = pain177d::threePatterns();
    s.patterns[0].lengthTicks = 3840;     // "Bass", 1 bar
    s.patterns[1].lengthTicks = 4 * 3840; // "drums", 4 bars
    s.patterns[2].lengthTicks = 2 * 3840; // "Arp", 2 bars
    auto bass = pain177d::clipOf (0, 0);
    bass.length = 4 * 3840; // stretched by hand to 4 bars
    s.playlist.push_back (bass);
    const auto before = yw::patternLengths (s);
    yw::sortPatterns (s, yw::PatternSort::name); // Arp, Bass, drums: Bass now sits where the 4-bar pattern was
    CHECK (s.playlist[0].pattern == 1 && s.playlist[0].length == 4 * 3840);
    auto followed = s;
    yw::followPatternLengths (before, followed);
    CHECK (followed.playlist[0].length == 3840); // what going through editSong did to the clip
}

TEST (pain177d_merge_identical_patterns)
{
    auto s = pain177d::threePatterns(); // every pattern holds the same note
    s.patterns[1].notes[0].key = 62;    // "drums" differs
    s.patterns[2].colour = 0xff00ff00;  // a different colour and name don't matter
    s.playlist.push_back (pain177d::clipOf (2, 0));
    s.playlist.push_back (pain177d::clipOf (1, 960));
    const auto groups = yw::identicalPatterns (s);
    CHECK (groups.size() == 1 && (groups[0] == std::vector<int> { 0, 2 }));
    CHECK (yw::mergeIdenticalPatterns (s) == 1);
    CHECK (s.patterns.size() == 2 && s.patterns[0].name == "Bass" && s.patterns[1].name == "drums");
    CHECK (s.playlist[0].pattern == 0 && s.playlist[1].pattern == 1);
    CHECK (s.currentPattern == 0); // the current pattern was the duplicate
    // another Loop Starter genre is another pattern
    auto g = pain177d::threePatterns();
    g.patterns[1].notes = g.patterns[0].notes;
    g.patterns[2].notes = g.patterns[0].notes;
    g.patterns[2].loopStarterGenre = "House";
    CHECK (yw::identicalPatterns (g).size() == 1 && (yw::identicalPatterns (g)[0] == std::vector<int> { 0, 1 }));
    // empty patterns are never merged
    auto e = pain177d::threePatterns();
    for (auto& p : e.patterns)
        p.notes.clear();
    CHECK (yw::identicalPatterns (e).empty());
}
