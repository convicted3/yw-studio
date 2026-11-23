// Round 190ao (pain point 13): "Play a note when a preset loads" — when the short note plays.

TEST (audition190ao_plays_only_for_a_stopped_instrument_with_the_option_on)
{
    using yw::pluginpresets::auditionPlays;
    CHECK (auditionPlays (true, true, false, false));
    CHECK (! auditionPlays (false, true, false, false)); // the option is off (the default): nothing plays
    CHECK (! auditionPlays (true, false, false, false)); // an effect has no note to play
    CHECK (! auditionPlays (true, true, true, false));   // the song plays: the song is the audition
    CHECK (! auditionPlays (true, true, false, true));   // recording: no extra note in the take
    CHECK (! auditionPlays (true, true, true, true));
}

TEST (audition190ao_note_is_a_short_c5_at_fls_default_velocity)
{
    CHECK (yw::pluginpresets::auditionKey == 60); // C5 in FL's octave numbering
    CHECK (std::abs (yw::pluginpresets::auditionVelocity - 100.0f / 127.0f) < 1.0e-6f);
    CHECK (yw::pluginpresets::auditionDelayMs > 0 && yw::pluginpresets::auditionDelayMs <= 250);
    CHECK (yw::pluginpresets::auditionLengthMs >= 200 && yw::pluginpresets::auditionLengthMs <= 1000);
}
