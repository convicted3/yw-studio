// Issue #117: the typing piano plays from YW windows too (Sytrus, effects, tools), not only the main window.

TEST (keys117_typing_piano_plays_from_yw_windows_unless_a_text_field_types)
{
    using yw::keys::KeyMap;
    // issue #117: with "Typing keyboard to piano" on, Z..M sounded only while the Channel rack had the focus, not over
    // Sytrus's window. A key the piano takes plays from any YW window; a focused text field keeps typing.
    CHECK (KeyMap::secondaryWindowPlaysNote (true, false));
    CHECK (! KeyMap::secondaryWindowPlaysNote (true, true));
    CHECK (! KeyMap::secondaryWindowPlaysNote (false, false)); // piano off, or a key its layout doesn't take
    // what is handed over: the keys the layout takes ('z' plays a note), not FL's FREE ones ('a' stays a shortcut)
    const auto piano = yw::keys::typingpiano::pianoLayout();
    CHECK (KeyMap::secondaryWindowPlaysNote (yw::keys::typingPianoKey ('z', 4, 0, piano).taken, false));
    CHECK (! KeyMap::secondaryWindowPlaysNote (yw::keys::typingPianoKey ('a', 4, 0, piano).taken, false));
}
