// UI-D (second UI polish round) tests. Included at the end of tests.cpp.

#include "core/EditorPolish.h"

TEST (polish_stretch_notes)
{
    using yw::Note;
    std::vector<Note> notes (2);
    notes[0].start = 100;
    notes[0].length = 50;
    notes[1].start = 200;
    notes[1].length = 100; // span 100..300

    // twice as long: starts and lengths double around the anchor
    auto out = yw::polish::stretchNotes (notes, 100, 300, 500);
    CHECK (out[0].start == 100 && out[0].length == 100);
    CHECK (out[1].start == 300 && out[1].length == 200);

    // half: squeezed, never shorter than the minimum
    out = yw::polish::stretchNotes (notes, 100, 300, 200, 30);
    CHECK (out[0].start == 100 && out[0].length == 30);
    CHECK (out[1].start == 150 && out[1].length == 50);

    // a degenerate span leaves the notes alone
    out = yw::polish::stretchNotes (notes, 300, 300, 600);
    CHECK (out[0].start == 100 && out[1].length == 100);

    // the end never crosses the anchor
    out = yw::polish::stretchNotes (notes, 100, 300, 50);
    CHECK (out[0].start >= 100 && out[1].start >= 100 && out[0].length >= 1);
}

TEST (polish_drum_paint_cells)
{
    yw::Pattern p;
    yw::Note n;
    n.channel = 1;
    n.key = 36;
    n.start = 96;
    p.notes.push_back (n);
    CHECK (yw::polish::hasNoteInCell (p, 1, 36, 96, 24));
    CHECK (yw::polish::hasNoteInCell (p, 1, 36, 90, 24));   // starts inside [90, 114)
    CHECK (! yw::polish::hasNoteInCell (p, 1, 36, 120, 24));
    CHECK (! yw::polish::hasNoteInCell (p, 2, 36, 96, 24)); // another channel
    CHECK (! yw::polish::hasNoteInCell (p, 1, 38, 96, 24)); // another key
}

TEST (polish_clip_ghost_notes)
{
    yw::Song song;
    song.beatsPerBar = 4;
    song.patterns.resize (3);
    const auto bar = (std::int64_t) yw::ppq * 4;

    yw::Note a; // pattern 1, channel 0, at the start of its second beat
    a.channel = 0;
    a.key = 60;
    a.start = yw::ppq;
    a.length = yw::ppq;
    song.patterns[1].notes.push_back (a);
    yw::Note other = a; // another channel: not a ghost of channel 0
    other.channel = 3;
    song.patterns[1].notes.push_back (other);

    // pattern 0 (the one being edited) plays bars 2-3; pattern 1 (one bar long) has a clip stretched over bars 1-4,
    // which plays it once in FL (round 168k: no repeats → nothing in bars 2-3) and loops it with the YW extra
    // "Stretched pattern clips loop their pattern" (on by default), and a 1-bar clip at bar 3
    yw::Clip home;
    home.pattern = 0;
    home.start = bar;
    home.length = 2 * bar;
    yw::Clip clip1;
    clip1.pattern = 1;
    clip1.start = 0;
    clip1.length = 4 * bar;
    yw::Clip clip2;
    clip2.pattern = 1;
    clip2.start = 2 * bar;
    clip2.length = bar;
    clip2.track = 2;
    song.playlist = { clip1, home, clip2 };

    const auto wasLooping = yw::stretchedClipsLoop();
    yw::setStretchedClipsLoop (false); // FL's behaviour (the YW extra off)
    auto ghosts = yw::polish::clipGhostNotes (song, 0, 0);
    CHECK (ghosts.size() == 1); // only the clip at bar 3
    CHECK (ghosts.size() == 1 && ghosts[0].start == bar + yw::ppq);
    CHECK (ghosts.size() == 1 && ghosts[0].key == 60 && ghosts[0].channel == 0);

    // not in the Playlist: nothing
    CHECK (yw::polish::clipGhostNotes (song, 2, 0).empty());

    // the YW extra on (the default, issue #25): the stretched clip loops its pattern, so its passes in bars 2 and 3
    // are ghosts too
    yw::setStretchedClipsLoop (true);
    song.playlist[2].muted = true;
    ghosts = yw::polish::clipGhostNotes (song, 0, 0);
    CHECK (ghosts.size() == 2 && ghosts[0].start == yw::ppq && ghosts[1].start == bar + yw::ppq);
    CHECK (ghosts.size() == 2 && ghosts[0].key == 60 && ghosts[0].channel == 0);

    // a muted clip is not heard: no ghosts from it
    yw::setStretchedClipsLoop (false);
    CHECK (yw::polish::clipGhostNotes (song, 0, 0).empty());
    song.playlist[0].muted = true;
    yw::setStretchedClipsLoop (true);
    CHECK (yw::polish::clipGhostNotes (song, 0, 0).empty());
    yw::setStretchedClipsLoop (wasLooping);
}

TEST (polish_track_layout)
{
    yw::polish::TrackLayout layout;
    layout.set (1, 60);
    layout.set (3, 5); // clamped to the minimum
    CHECK (layout.heightOf (0) == 38 && layout.heightOf (1) == 60 && layout.heightOf (3) == yw::polish::minTrackHeight);
    CHECK (layout.top (0) == 0 && layout.top (1) == 38 && layout.top (2) == 98);
    CHECK (layout.trackAt (37, 10) == 0 && layout.trackAt (38, 10) == 1 && layout.trackAt (97, 10) == 1 && layout.trackAt (98, 10) == 2);
    CHECK (layout.trackAt (100000, 10) == 9 && layout.trackAt (-5, 10) == 0);
    CHECK (layout.total (2) == 98);
    layout.setAll (50);
    CHECK (layout.heightOf (1) == 50 && layout.top (2) == 100);
}

TEST (polish_make_clips_unique)
{
    yw::Song song;
    song.patterns.resize (2);
    song.patterns[0].name = "Beat";
    song.patterns[1].name = "Beat #2"; // taken: the copy becomes "Beat #3"
    yw::Clip c;
    c.pattern = 0;
    song.playlist = { c, c, c };

    yw::polish::makeClipsUnique (song, { 1, 2 });
    CHECK (song.patterns.size() == 3);
    CHECK (song.patterns[2].name == "Beat #3");
    CHECK (song.playlist[0].pattern == 0 && song.playlist[1].pattern == 2 && song.playlist[2].pattern == 2);
    CHECK (song.currentPattern == 2);

    // automation clips get their own automation
    yw::Automation a;
    a.name = "Cutoff";
    song.automations.push_back (a);
    yw::Clip ac;
    ac.kind = yw::ClipKind::automation;
    ac.automation = 0;
    song.playlist.push_back (ac);
    yw::polish::makeClipsUnique (song, { 3, 99 }); // out-of-range indices are ignored
    CHECK (song.automations.size() == 2 && song.automations[1].name == "Cutoff #2" && song.playlist[3].automation == 1);
}

TEST (polish_remember_seek)
{
    CHECK (yw::polish::stopReturnTick (false, 960) == 0);
    CHECK (yw::polish::stopReturnTick (true, 960) == 960);
    CHECK (yw::polish::stopReturnTick (true, -1) == 0);
}
