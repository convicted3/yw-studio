# Contributing to YW Studio

Thanks for wanting to help! YW Studio is an alpha, so bug reports, ideas and pull requests are all very welcome.

## Reporting bugs and asking for features
- Use the [issue templates](https://github.com/convicted3/yw-studio/issues/new/choose): *Bug report*, *Feature request* or *Sound / plugin difference*.
- Say which YW Studio version and OS you use (Help › About). For crashes, attach the crash file YW Studio offers to send, or `%APPDATA%\YW Studio\autosave\crash*.txt` (Windows) / `~/.config/YW Studio/autosave/` (Linux).
- Screenshots and short videos help a lot, especially side by side with FL Studio.

## Pull requests
1. Build YW Studio as described in [README › Building](README.md#building) and run the tests (`scripts/build-local.sh`). They must pass with 0 failures.
2. Keep each pull request focused on one change, and add a test for logic changes.
3. Code, UI texts, comments and docs are in **English**. New UI texts also need a pt-BR line in `resources/i18n/pt-BR.txt`.
4. Follow the style of the file you're editing (C++20, JUCE 8).

## Clean-room rule
YW Studio recreates the FL Studio *workflow*, not its code or artwork.
- Don't copy code, graphics, presets, samples or names from FL Studio or any third-party plugin.
- Third-party plugins are studied as black boxes (host them, feed signals, measure). Since 2026-11, by the project owner's decision, some stand-ins were also informed by clean-room static analysis of the third-party binaries (documented in [`docs/analise/REA_*.md`](docs/analise)); that is done only by the owner's decision, and nothing found that way is copied — no code, tables, presets or other content. Never patch a third-party plugin or bypass its licence protection.

## License
By contributing you agree that your contribution is licensed under the [GNU AGPL v3](LICENSE), like the rest of YW Studio.
