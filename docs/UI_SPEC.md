# YW Studio — спецификация интерфейса v0.1

Справочник для разработки. Описывает раскладку, меню и поведение интерфейса по образцу FL Studio, собранные из официального онлайн-мануала Image-Line (FL Studio 2025/2026). Графика, иконки, цвета, шрифты и фирменные названия — свои, YW.

**Правила clean-room**
- Повторяем раскладку, порядок меню, хоткеи и поведение мыши.
- Не копируем графику, иконки, скины, звуки, пресеты и код FL.
- Не используем названия-бренды Image-Line (Fruity, Edison, Gopher, FL Cloud, Riff Machine и т.п.) — у нас свои.
- Облачные, магазинные и лицензионные функции FL (FL Cloud, Unlock, Shop, мастеринг) не повторяем.

**Метки приоритета**
- **[v1]** — первая рабочая версия
- **[v2]** — следующие этапы
- **[—]** — не делаем / заменяем своим

---

## 0. Карта названий FL → YW

| FL Studio | YW Studio (предложение) | Примечание |
|---|---|---|
| FL Studio | YW Studio | |
| Channel Rack | Channel Rack | общий термин |
| Piano roll | Piano Roll | общий термин |
| Playlist | Playlist | общий термин |
| Mixer | Mixer | |
| Browser | Browser | |
| Plugin Picker | Plugin Picker | |
| Plugin Manager | Plugin Manager | |
| Plugin database | Plugin Library | |
| Edison | YW Wave (позже) | свой аудиоредактор, [v2] |
| Riff Machine | Idea Generator | [v2] |
| Gopher | — | не делаем |
| Fruity Wrapper | Plugin Host | окно-обёртка плагина |
| Channel Sampler | YW Sampler | наш встроенный сэмплер [v1] |
| FL Cloud, Shop, Unlock | — | не делаем; вместо этого «Каталог плагинов» (OwlPlug + yolo.su) |
| .flp | .yws | свой формат проекта; .flp — позже |
| .fst (пресет канала) | .ywc | |
| .fsc (score) | .ywn | ноты; плюс импорт/экспорт MIDI |

---

## 1. Главное окно

Сверху вниз:
1. **Строка заголовка**: логотип YW, имя проекта, кнопки окна (свернуть/развернуть/закрыть).
2. **Главное меню**: File · Edit · Add · Patterns · View · Options · Tools · Help.
3. **Тулбар** из панелей (порядок слева направо, все можно скрыть или переставить):
   1. Hint bar — подсказка при наведении на контрол, значение при изменении [v1]
   2. Transport: PAT/SONG, Play/Pause, Stop, Record [v1]
   3. Tempo: крупные цифры BPM (10–522) + мелкие дробные [v1]
   4. Song position — слайдер позиции [v1]
   5. Time display — Bar:Step:Tick или Min:Sec:Cs [v1]
   6. Master volume + Master pitch [v1]
   7. Global Snap [v1]
   8. Pattern selector [v1]
   9. Add (кнопка меню Add) [v1]
   10. Multilink to controllers [v2]
   11. Output: осциллограф/спектр + пиковый индикатор [v1 — пик, v2 — остальное]
   12. CPU / Memory / полифония [v1]
   13. Shortcut icons: кнопки открытия Playlist, Piano Roll, Channel Rack, Mixer, Browser, Plugin Picker [v1]
4. **Рабочая область**: Browser слева (докуется), остальное — плавающие/докуемые окна внутри главного.

Правый клик по тулбару: Edit (настройка раскладки), Separators, Flat buttons, Hint bar, Hide (Ctrl+F11), Presets. [v2]

### 1.1 Транспорт
| Контрол | Хоткей | Поведение |
|---|---|---|
| PAT/SONG | L | режим паттерна / песни. ПКМ по PAT — открыть Channel Rack, по SONG — Playlist |
| Play/Pause | Space / Ctrl+Space | Space: старт и стоп с возвратом; при включённой записи начинает запись |
| Stop | Space | двойной клик — panic (снять все ноты) |
| Record | R | Alt+клик — опции записи; ПКМ — фильтр записи (automation, notes, audio, …) [v2] |
| Tempo | — | драг вверх/вниз; ПКМ: Type in value, Tap tempo, пресеты 80–160, Copy/Paste value |

### 1.2 Global Snap
Варианты: Line, Cell, (none), Steps 1/6 · 1/4 · 1/3 · 1/2 · 1 step, Beats 1/6 · 1/4 · 1/3 · 1/2 · 1 beat, Bar. Alt при перетаскивании временно отключает снап. ПКМ — длина ноты по умолчанию. Окна с локальным снапом «Main» используют это значение.

### 1.3 Pattern selector
Имя паттерна + кнопки +/−. ПКМ — список всех паттернов. Максимум 999 паттернов. Меню: найти пустой, переименовать/цвет, клонировать, удалить, вверх/вниз, разделить по каналам, рендер в аудио.

---

## 2. Главное меню

### File
| Пункт | Хоткей | Приоритет |
|---|---|---|
| New | — | v1 |
| New from template ▸ (группы шаблонов) | — | v2 |
| Open… | Ctrl+O | v1 |
| Save | Ctrl+S | v1 |
| Save as… | Shift+Ctrl+S | v1 |
| Save new version | Ctrl+N | v1 |
| Save as template… | — | v2 |
| Import ▸ MIDI file…, (позже) FL Studio project (.flp) | — | MIDI v1, FLP v2 |
| Export ▸ WAV (Ctrl+R), MP3 (Shift+Ctrl+R), OGG, FLAC, MIDI (Shift+Ctrl+M), All playlist tracks (stems), Zipped project (проект + сэмплы), Project data files | — | WAV/MIDI v1, остальное v2 |
| Recent projects ▸ (10, «More» до 50) | Alt+1…0 | v1 |
| Revert to last backup | — | v1 |
| Exit | Alt+F4 | v1 |

### Edit
| Пункт | Хоткей |
|---|---|
| Undo / Redo (шаг вперёд-назад по истории) | Ctrl+Z, Ctrl+Alt+Z |
| Undo history | — |
| Cut / Copy / Paste (ноты выбранных каналов в step sequencer) | Ctrl+X / C / V |
| Delete | Del |

### Add
1. Channel ▸ (избранные инструменты, More plugins…) [v1]
2. Effect ▸ (в выбранный трек микшера) [v1]
3. View plugin picker [v1]
4. Browse plugin library [v1]
5. Browse all installed plugins [v1]
6. Browse presets [v2]
7. Refresh plugins list (fast scan) [v1]
8. Manage plugins… [v1]
9. Automation clip for last tweaked parameter [v1]
10. Pattern [v1]

### Patterns
| Пункт | Хоткей |
|---|---|
| Find first empty | Shift+F4 |
| Find next empty | F4 |
| Find next empty (no naming) | Ctrl+4 |
| Select in playlist | — |
| Rename and color… | F2 |
| Random color | — |
| Open in project browser | Shift+Ctrl+P |
| Group selected / Ungroup selected | — |
| Set time signature… | — |
| Transpose… | — |
| Insert one | Shift+Ctrl+Ins |
| Clone | Alt+C |
| Delete | Del |
| Move up / Move down | Shift+Ctrl+Up / Down |
| Split by channel | — |
| Quick render as audio clip / Render as audio clip / Render and replace | — (v2) |

### View
**Windows**
| Пункт | Хоткей |
|---|---|
| Playlist | F5 |
| Channel rack | F6 |
| Piano roll | F7 |
| Plugin picker | F8 |
| Browser | Alt+F8 |
| Project picker | Ctrl+F8 (v2) |
| Mixer | F9 |
| Tempo tapper | — |
| Toolbars ▸ Edit, Separators, Hint bar, Flat buttons, Hide (Ctrl+F11) | — |
| Plugin performance monitor | — (v2) |
| Welcome screen | — |

**Layout**
| Пункт | Хоткей |
|---|---|
| Close all windows | F12 |
| Close all plugin windows | Alt+F12 |
| Close all unfocused windows | Ctrl+F12 |
| Align all channel editors | Shift+F12 |
| Arrange windows ▸ Realign (Shift+Ctrl+H), Into workspace, Default, Alternate, Custom, Save custom layout | — |
| Background ▸ Set image, Set color | — (v2) |

**Browser**: Undo history, Patterns, Generators in use, Effects in use, Plugin library (Shift+F8).

### Options
**System**: MIDI settings (F10), Audio settings, General settings, File settings, Theme settings, Manage plugins.
**Project**: Project info (F11), Project general settings.
**Info**: Debugging log.
**MIDI**: Enable MIDI remote control, Enable MIDI output, Enable MIDI master sync. (v2)
**Switches**:
| Пункт | Хоткей |
|---|---|
| Typing to piano keyboard | Ctrl+T |
| Metronome | Ctrl+M |
| Recording precount | Ctrl+P |
| Start on input | Ctrl+I |
| Blend recorded notes | Ctrl+B |
| Step edit | Ctrl+E |
| Loop record | — |
| Auto scrolling | — |
| Enable groups | — |
| Multi-link to controllers | Ctrl+J |

### Tools
1. Browser find — Ctrl+F [v1]
2. One-click audio recording ▸ Into Playlist as audio clip [v2]
3. Idea generator (аналог Riff machine) [v2]
4. Chord progression tool [v2]
5. Macros ▸
   - Panic ▸ Stop sound (Ctrl+H) [v1], Cancel recording
   - Misc ▸ Select unused channels, Purge unused audio clips, Switch smart disable for all plugins, Lock/Unlock all playlist tracks to content, Reset empty playlist tracks, Delete unused files and save project, Switch all plugin editors to Attached/Detached
6. Clipboard ▸ Save audio/MIDI clipboard to browser [v2]
7. Score logger ▸ Dump score log to selected pattern (последние 2/5/10/20/30 мин), Clear log [v2]
8. Audio logger ▸ Enable, Maximum length, Recording position [v2]
9. Last tweaked parameters ▸ Edit events, Create automation clip, Link to controller, Init song with this position, Copy/Paste value [v1 частично]
10. External tools… [v2]
11. **Каталог плагинов YW** (наш пункт: OwlPlug + список yolo.su) [v2]

### Help
Help index (F1) — открывает документацию на yolo.su · Tutorials · What's new · Check for updates · Сайт YW Studio · GitHub (issues/исходники) · About.

---

## 3. Channel Rack (F6)

### 3.1 Заголовок (слева направо)
1. **Меню опций** (стрелка). Пустая часть заголовка — за неё таскаем окно; Shift+колесо — листать паттерны.
2. **Display filter**: All / Unsorted / пользовательские группы. PgUp/PgDn — следующая/предыдущая группа. ПКМ: Add filter group, Rename, Delete.
3. **Global swing** — ручка 0–100%, сдвигает каждый второй шаг 1/16.
4. **Pattern length**: Auto или 1–512 шагов; ПКМ — в тактах. Угол окна можно тянуть, чтобы показать больше шагов.
5. **Loop** — переключатель и режимы по ПКМ.
6. **Graph editor** (Ctrl+K).
7. (Loop Starter у FL — [—], не делаем.)

### 3.2 Строка канала (слева направо)
1. **Mute LED**: ЛКМ — mute; Ctrl/Alt+клик или ПКМ ▸ Solo — solo; Alt+клик по группе — вся группа; Shift+клик — заблокировать состояние.
2. **Pan** — ручка.
3. **Volume** — ручка (сэмплер по умолчанию ≈78% ≈ −5.2 dB).
4. **Mixer track** — номер трека, «---» = Master. Скрыт по умолчанию, показать Alt+M.
5. **Channel button**: имя + цвет. ЛКМ — открыть/закрыть окно канала/плагина; Alt+ЛКМ — открыть, не закрывая другие; Ctrl+ЛКМ — прослушать; ПКМ — меню канала.
6. **Selector**: внешняя рамка — выделение, внутренняя часть — индикатор активности. ЛКМ — выбрать; ПКМ — переключить независимо; ПКМ+драг вниз — диапазон; двойной клик — выбрать все.
7. **Step buttons** — по шагу 1/16. ЛКМ — включить, ПКМ — выключить (опция «Use both mouse buttons»). Драг — рисовать шагами.
8. **Мини-превью Piano Roll** вместо шагов, если в канале есть ноты: ЛКМ — открыть Piano Roll, ПКМ — открыть с центрированием на нотах.
9. **Zipped** — компактная строка; ПКМ — развернуть.

Внизу: кнопка **«+» (Add a plugin)** — избранные инструменты и More plugins….

### 3.3 Меню опций Channel Rack
1. Add one ▸ (избранные, More plugins…)
2. Select unused channels
3. Clone selected — Alt+C
4. Delete selected — Alt+Del
5. Move selected up / down — Alt+Up / Alt+Down
6. Sort by ▸ Color, Name, Track number, Type
7. Group selected — Alt+G
8. Color selected ▸ Gradient, Random
9. Mute selected / Unmute selected
10. Transpose selected…
11. Set swingmix for selected ▸ 0/25/50/75/100%/Custom
12. Assign selected to free mixer track(s) — Ctrl+L
13. Zip selected — Alt+Z; Unzip all — Alt+U
14. Auto resize ▸ Off, Minimum, Maximum
15. Channel button width ▸ 100–400%
16. Channel name alignment ▸ Centered, Left
17. Focus selected channel on playback
18. Show mixer track selectors — Alt+M
19. Show complete piano roll preview
20. Detached

### 3.4 Меню кнопки канала (ПКМ)
1. Piano roll / Send to piano roll (если есть шаги)
2. Graph editor
3. Rename, recolor and icon… (F2) · Random color · Change color · Change icon
4. Load sample… (для сэмплера)
5. Cut itself
6. Insert ▸ (избранные, More…)
7. Replace ▸ (избранные, More…)
8. Clone · Delete
9. Cut / Copy / Paste (ноты)
10. Fill each 2 / 4 / 8 steps · Advanced fill…
11. Rotate left (Shift+Ctrl+Left) · Rotate right (Shift+Ctrl+Right)
12. Receive notes from ▸ (контроллер / MIDI-канал) [v2]
13. Burn MIDI to ▸ current / new pattern [v2]
14. Reorder (Alt+Up/Down, Shift+колесо)

### 3.5 Graph editor (вкладки)
Note · Velocity · Release · Fine pitch (±100 центов) · Pan · Mod X · Mod Y · Shift (сдвиг старта) · Rep (повтор). ЛКМ+драг — значение, Ctrl — масштаб всех, ПКМ-драг — линия между значениями, Alt+клик — сброс.

---

## 4. Окно канала / Plugin Host

### 4.1 Заголовок обёртки плагина
1. Меню плагина (стрелка)
2. Пресеты: имя текущего + стрелки ◀ ▶; ПКМ — меню
3. Кнопка настроек обёртки (шестерёнка)
4. Typing keyboard priority (если плагин принимает клавиатуру)
5. Вернуться к интерфейсу плагина
6. Свернуть / закрыть

### 4.2 Меню плагина
1. Piano roll
2. Replace ▸ · Delete
3. Presets ▸ (Randomize — v2) · Browse presets · Save preset as…
4. Add to plugin library (favorite)
5. Display preset name in selector
6. Spare state ▸ Store, Flip
7. Parameters ▸ Link all parameters…, Browse parameters
8. CPU ▸ Allow threaded processing, Smart disable
9. Misc ▸ Rename/color/icon (F2), Scaling, Detached, Help
10. Channel ▸ Save channel state as…, Browse channel states, Route to free mixer track (Ctrl+L)

### 4.3 Детальные настройки обёртки
**Settings**: Info (имя, формат VST2/VST3/CLAP, 32/64 бит, задержка + ручная поправка) · Options (Autosave, Reset) · Presets (загрузить/сохранить FXP/FXB для VST2, .vstpreset для VST3) · Reload plugin, Refresh properties · MIDI (порты in/out, release velocity, pitch bend range, All Notes Off при стопе) · GUI (DPI aware, Scale editor, Keep focus, Accept dropped files) · Automation (Notify about parameter changes).
**Processing**: Threaded processing · Smart disable · **Run in separate process (bridge/песочница)** · Process inactive I/O · Connections (Auto map inputs/outputs, ручная маршрутизация выходов на треки микшера).
**Troubleshooting**: Reset on transport · Send loop position · Fixed size buffers · Fast idle · Ctrl+Z → undo YW · Forward unused keys to YW.

### 4.4 YW Sampler (встроенный канал для сэмплов) [v1]
Сэмпл (драг из браузера), громкость, панорама, питч, высота ноты (root), реверс, старт/конец, луп, огибающая громкости ADSR, фильтр (cutoff/res, Mod X/Mod Y), declick, cut/cut by, выбор трека микшера. Вкладки: Sample · Envelope · Misc.

---

## 5. Piano Roll (F7)

### 5.1 Раскладка
Заголовок с выбором канала (Target channel) → тулбар → линейка времени с маркерами → сетка нот с клавиатурой слева → разделитель → Event editor (нижняя полоса) → зум и скроллы. Опционально — панель ghost channels.

### 5.2 Тулбар (по порядку)
| # | Инструмент | Хоткей | Поведение |
|---|---|---|---|
| 1 | Меню Piano Roll | — | |
| 2 | Tools (гаечный ключ) | — | меню инструментов; ПКМ — chord progression (v2) |
| 3 | Snap | — | ЛКМ — снап; ПКМ — длина ноты по умолчанию |
| 4 | Snap to scale | — | ПКМ: root, scale, snap PR notes, snap incoming MIDI, shading |
| 5 | Stamp (аккорды) | — | выбрать аккорд → клик ставит аккорд |
| 6 | Draw | P | ЛКМ — нота, драг — двигать, ПКМ — удалить |
| 7 | Paint | B | драг — рисует много нот |
| 8 | Paint drums | N | шаги драгом; ЛКМ по ноте — mute |
| 9 | Delete | D | |
| 10 | Mute | T | |
| 11 | Interpolate | I | для данных событий (v2) |
| 12 | Slice | C | вертикальный разрез; Ctrl+G — склеить |
| 13 | Select | E | |
| 14 | Zoom | Z | |
| 15 | Play selected | Y | прослушать/скраб |
| 16 | Play/Pause | — | |
| 17 | Portamento | O | новые ноты с флагом porta |
| 18 | Slide | S | новые ноты — slide |
| 19 | Note color | — | 16 цветовых групп |
| 20 | Ghost channels | — | показать/скрыть, ПКМ — меню |

### 5.3 Свойства ноты
Двойной клик по ноте: PAN, VEL, REL, MOD X, MOD Y, Fine pitch, Slide, Porta, Color, Time (bar:step:tick), Duration, Reset, Accept. Если нота в выделении — применяется ко всему выделению.

### 5.4 Event editor
Выбор цели (ЛКМ, колесо, Shift+F): Velocity, Pan, Release, Mod X, Mod Y, Fine pitch, плюс любой автоматизируемый параметр. Палочки с кружком. Shift+драг — задать диапазону одно значение, Alt+клик — сброс, Alt+колесо — изменить, Ctrl+Alt+колесо — точно.

### 5.5 Мышь и клавиатура
- Колёсико-драг — скролл; Ctrl+ПКМ-драг — зум; PgUp/PgDn — зум у курсора; Shift+1/2/3 — быстрый зум, Shift+4 — всё, Shift+5 — выделение.
- Ctrl+драг — выделение; Ctrl+Shift — добавить/убрать; Shift+I — инвертировать; Ctrl+клик по пустому — снять.
- Alt — без снапа; Shift+драг ноты — только по времени; Ctrl+драг — только по высоте.
- Тянуть правый край — длина; левый край — опция меню.
- Shift+стрелки — сдвиг на шаг снапа; Alt+стрелки — точно.
- Ctrl+B — дублировать вправо; Shift+G — группа.
- Клик по клавише — прослушать; Ctrl+клик по клавише — выделить все ноты этой высоты.
- Двойной ПКМ по ghost-ноте — перейти на её канал.

### 5.6 Меню Piano Roll
**File**: Open score… · Save score as… · Import MIDI file… (Ctrl+M) · Export as MIDI file… (Shift+Ctrl+M) · Paste from MIDI clipboard (Shift+Ctrl+V) · Copy to MIDI clipboard.
**Edit**: Cut · Copy · Paste · Duplicate (Ctrl+B) · Delete · Shift left/right (Shift+←/→) · Rotate left/right (Shift+Ctrl+←/→) · Transpose up/down (Shift+↑/↓) · Octave up/down (Ctrl+↑/↓) · Discard lengths (Shift+D) · Allow resizing from left (Ctrl+Alt+Home) · Nudge with mouse wheel · Change color (Alt+C) · Mute/Unmute · Insert space (Ctrl+Ins) · Slice & insert space (Ctrl+Alt+Ins) · Delete space (Ctrl+Del) · Trim selection · Insert current controller value (Ctrl+I) · Turn into automation clip.
**Tools**: Quick legato (Ctrl+L) · Articulate (Alt+L) · Quick quantize (Ctrl+Q) · Quick quantize start times (Shift+Q) · Quantize… (Alt+Q) · Quick chop (Ctrl+U) · Chop… (Alt+U) · Glue (Ctrl+G) · Arpeggiate (Alt+A) · Strum (Alt+S) · Flam (Alt+F) · Claw machine (Alt+W) · Limit (Alt+K) · Flip (Alt+Y) · Randomize (Alt+R) · Scale levels (Alt+X) · LFO (Alt+O) · Idea generator (Alt+E) · Chord progression (Alt+P) · Stamp. [v1: quantize, legato, chop, glue, flip, randomize; остальное v2]
**View**: Grid (color, contrast, invert, time segments) · Content (shadow, rounded, note colors/palette) · Time (precise indicator, mini preview) · Helpers (note grid highlights Alt+B, scale highlighting, ghost channels Alt+V, editable ghosts Ctrl+Alt+V, background waveform Alt+N) · Piano roll (keyboard style, keyboard labels, swap panels, flip separator Shift+Enter).
**Snap**: Main · Line · Cell · (none) · 1/6–1 step · 1/6–1 beat · Bar · Events · Markers.
**Select**: Deselect (Ctrl+D) · Select all (Ctrl+A) · Select random (Shift+R / Shift+M) · Select by color (Shift+C) · Select odd (Shift+O) · Select muted · Select overlapping · Invert (Shift+I) · Select time around selection (Ctrl+Enter) · Previous/next time (Ctrl+←/→) · Magic lasso.
**Group**: Group (Shift+G) · Ungroup (Alt+G).
**Zoom**: In/Out (PgUp/PgDn) · Quick 1/2/3 (Shift+1/2/3) · Out far (Shift+4) · On selection (Shift+5).
**Time markers**: Add one (Alt+T) · Add auto (Ctrl+T) · Add two · Add one every… · Time signature change · Key marker · Delete.
**Прочее**: Target channel · Target control · Auto locate channel · Auto smoothing · Preview notes during playback · Center (Shift+0) · Detached.

---

## 6. Playlist (F5)

### 6.1 Заголовок
Arrangements (Add one, Clone, Rename, Delete, Merge with…) [v2] · Clip source (выбор клипа для рисования; ПКМ — project picker) · Clip focus (Pattern / Audio / Automation) · «+» добавить трек · кнопки окна.

### 6.2 Тулбар
| # | Инструмент | Хоткей |
|---|---|---|
| 1 | Меню Playlist | — |
| 2 | Snap | — |
| 3 | Draw | P |
| 4 | Paint | B |
| 5 | Delete | D |
| 6 | Mute | T |
| 7 | Slip edit | S |
| 8 | Slice | C |
| 9 | Select | E |
| 10 | Zoom | Shift+Z |
| 11 | Play selected | Y |
| 12 | Play/Pause | — |

### 6.3 Picker panel (Alt+P)
Список всех клипов (паттерны, аудио, автоматизация) с фильтрами. Драг на плейлист или клик → клик по треку. Shift+клик — переименовать, Alt+ПКМ — прослушать, Shift+C — найти в плейлисте, Shift+колесо — порядок.

### 6.4 Линейка и время
ЛКМ по номеру такта — старт воспроизведения; Home — в начало. Ctrl+драг — выделение времени по всем трекам. Маркеры (Alt+T), Song loop, тактовый размер (Shift+Alt+T). Ctrl+Ins / Ctrl+Alt+Ins / Ctrl+Del — вставить/разрезать-вставить/удалить время. Ctrl+колесо — горизонтальный зум.

### 6.5 Заголовок трека
Selector · имя (клик — переименовать) · цвет (ЛКМ — выбор, ПКМ — случайный) · иконка · Mute (ЛКМ mute, Ctrl/ПКМ solo, Alt — группа, Shift — заблокировать) · разделитель (тянуть — высота, вверх в трек — группировка, Alt+клик — сброс) · режим трека (Instrument/Audio/Unassigned) + Arm/вход/мониторинг для аудиотреков.

**ПКМ по треку**: Rename/color · Set icon · Auto name · Auto name clips · Reset · Track mode ▸ · Size ▸ · Lock to this size · Group with above track · Auto color group · Lock to content · Merge pattern clips · Consolidate track ▸ From track start (Ctrl+Alt+C) / From song start (Ctrl+Alt+Shift+C) · Mute/Unmute all clips · Insert one · Delete · Clone (Shift+Ctrl+B) · Move up/down.

### 6.6 Клипы
- **Pattern** — двойной клик: Piano Roll; Alt+двойной клик: Channel Rack.
- **Audio** — фейды (Shift+F), zero-cross, stretch под темп (Shift+M).
- **Automation** — точки, кривые, ступени; размер — за заголовок клипа.

**Меню клипа** (уголок клипа): Preview · Select source · Channel settings · Edit pattern · Rename and recolor · Change color · Make unique · Select all similar · Delete · Use current time signature · Region ▸ Chop · Sample ▸ (audio: Make unique as sample, Edit sample, Detect tempo, Fit to tempo) · Automate ▸ (audio: Volume/Pan, Crossfade). Для автоматизации — Articulator ▸ Flip vertically, Scale levels, Normalize, Smooth, Turn all points smooth.

### 6.7 Меню Playlist
**Edit**: Cut · Copy · Paste · Duplicate (Ctrl+B) · Delete · Shift left/right/up/down (Shift+стрелки) · Allow resizing from left (Ctrl+Alt+Home) · Nudge with wheel · Mute/Unmute · Insert space (Ctrl+Ins) · Slice and insert space (Ctrl+Alt+Ins) · Delete space (Ctrl+Del) · Merge pattern clips (Ctrl+G) · Merge similar pattern clips (Shift+Ctrl+G) · Merge automation clips (Ctrl+Alt+G) · Pitch −1/+1 semitone (7/8) · Reverse audio clips (9) · Automatic crossfades · Snap fade handles.
**Tools, View, Snap, Select, Group (Shift+G), Zoom, Time markers, Performance mode [v2], Playhead, Detach** — по аналогии с Piano Roll. Quantize (Alt+Q), Locate clip (Shift+C), Project picker (Ctrl+F8).

### 6.8 Мышь
Shift — переключение Draw/Paint, копирование драгом, добавить в выделение. Ctrl — временный Select, выделение рамкой. Alt — без снапа; Alt+ПКМ — проиграть клип. ПКМ — удалить клип (в Draw/Paint). Колёсико-драг — скролл. Ctrl+Shift+ПКМ по клипу — зум на клип.

---

## 7. Mixer (F9)

### 7.1 Раскладка
- **Master** + **Insert-треки** (в FL до 500; у нас динамически, по умолчанию 125) + **Current (C)** трек [v2].
- Доки: левый, средний (скроллится), правый.
- Пресеты вида: от Compact до Extra large (8 вариантов).
- Тулбар (слева направо): меню микшера · meter/wave view (Alt+W) [v2] · extra track properties · track inspector (панель FX) · add insert track(s).

### 7.2 Полоса трека (сверху вниз)
1. Номер и имя (ЛКМ — выбрать, колёсико-клик или F2 — переименовать)
2. Пиковые индикаторы (dB)
3. Иконки: invert phase · swap L/R · FX on/off (ПКМ — открыть все окна FX) · track delay/PDC · record arm
4. Mute/Solo: ЛКМ mute, ПКМ solo, Ctrl+ПКМ — unmute all, Shift — lock, Alt — solo с маршрутами
5. Pan
6. Stereo separation
7. Fader (зелёный — выбран, оранжевый — armed)
8. Внизу — переключатели посылов/стрелки маршрутизации на выбранный трек (ЛКМ — создать посыл; после — ручка уровня; ПКМ — sidechain only)

### 7.3 Track inspector (справа)
1. Вход (audio input) [v2]
2. **10 FX-слотов**: стрелка слева — ЛКМ select/ПКМ plugin picker; ЛКМ по имени — открыть; колесо над слотом — порядок; LED — mute/solo/lock; ручка Mix (dry/wet).
3. Меню слота: Replace · Delete · Presets · Browse presets · Save preset as · Add to plugin library · Store/Flip spare state · Link all parameters · Allow threaded processing · Smart disable · Move up/down · View editor · Rename/color/icon (F2) · Help.
4. 3-полосный EQ (ЛКМ-драг: частота/усиление; ПКМ-драг: ширина/усиление) [v2]
5. Задержка трека / PDC
6. Выход (audio output)

### 7.4 Меню микшера
1. Disk recording ▸ Render armed tracks (Alt+R), Render selected tracks (Shift+Alt+R), Auto-unarm, Auto-create audio clips, Latency compensation, 16/24/32 bit [v2]
2. Plugin delay compensation ▸ Automatic, Reset manual latency, Compensate automations
3. Link all parameters…
4. View ▸ Waveforms, Names at top, Routing cables, Compact plugin list, Track inspector on left, Colorful mixer, Lines between tracks
5. Detached

### 7.5 ПКМ по треку
Rename, color & icon (F2) · Assign/Unassign audio track · File ▸ Open/Save mixer track state, Browse states · Reset track(s) · Routing ▸ (Route selected channels to this track — Ctrl+L; Route starting from this track — Shift+Ctrl+L; Route/Sidechain selected to this track (only); Create submix; Route/Sidechain this track to selected (only); Select ▸ channels routed here — Alt+L, tracks routed to this — Ctrl+Alt+L; Reset routing) · Solo (S) · Alt solo (Alt+S) · Group ▸ Separator, Create group, Auto color · Layout ▸ Dock to, Move left/right (Shift+←/→), Delete, Insert, Trim unused tracks.

### 7.6 Мышь
Ctrl+Shift+клик — мультивыделение; Ctrl+A — все. Alt+←/→ или Shift+колесо — переставить трек. Колесо над именами — скролл. Почти любой контрол: ПКМ — автоматизация / привязка к контроллеру / edit events.

---

## 8. Browser (Alt+F8)

### 8.1 Шапка
Меню опций · Collapse all · Refresh (Ctrl+R) · Свернуть · Закрыть. Разделитель — тянуть ширину. Докуется к левому или правому краю.

### 8.2 Вкладки
All · Current project · Plugin library · Starred (избранное). Пользовательские вкладки: rename, color, hide, clone, reorder, delete.

### 8.3 Поиск
Поле поиска (без регистра, файлы и папки), фильтр по типу, теги (Any/All), Clear. Wildcards и AND/OR/NOT. Ctrl+F — найти, F3 — следующее, Esc — отмена.

### 8.4 Дерево по умолчанию
Current project (History, Patterns, Generators, Effects, Remote control, Samples) · Channel presets · Mixer presets · Plugin presets · Projects · Recorded · Rendered · Samples (свои папки) · Пользовательские папки (File settings).

### 8.5 Предпрослушка
ЛКМ по аудио — прослушать (по умолчанию 5 с, опция — целиком). Alt+клик — до конца. Shift+клик — с позиции мыши. Sync под темп/тональность, если в файле есть метаданные [v2]. Громкость предпрослушки.

### 8.6 Меню файла (ПКМ)
Send to selected channel · Open in new channel · Add to plugin library (для пресетов) · Open in new sampler · System ▸ Open parent folder, Locate file, Delete file · Tags ▸ Favorite, Add tag, Edit tags.

### 8.7 Меню папки (ПКМ)
Expand all / Collapse all · Open in new tab · Add folder to tab · Change color · Change icon · Open in Explorer.

### 8.8 Drag & drop
Сэмпл → Channel Rack (новый сэмплер) / Playlist (аудиоклип) / трек плейлиста / окно сэмплера · Инструмент → Channel Rack или трек микшера · Эффект → трек микшера или FX-слот · Проект → рабочая область (открыть) · Внешняя папка → браузер (добавить). Shift+↑/↓ — следующий/предыдущий файл в выбранный канал; Enter — заменить.

---

## 9. Плагины

### 9.1 Plugin Picker (F8)
Сетка/список: эффекты слева, инструменты справа. Поиск. Драг в проект. Избранное.

### 9.2 Plugin Manager (Options ▸ Manage plugins)
*Раздел мануала FL неполный, часть восстановлена по памяти — уточнить при разработке.*
- **Слева — Plugin search paths**: список папок (по умолчанию `C:\Program Files\Common Files\VST3`, `C:\Program Files\Common Files\CLAP`, `C:\Program Files\VSTPlugins`, `C:\Program Files\Steinberg\VSTPlugins`) + Add / Remove.
- **Опции сканирования**: Find installed plugins · Verify plugins (загрузить и проверить) · Rescan previously verified · Rescan plugins with errors · Combine VST and VST3 (показывать один плагин, если есть оба формата) · **Start scan**.
- **Справа — таблица**: ★ избранное · Name · Vendor · Type (Generator/Effect) · Format (VST2/VST3/CLAP) · Bit (64/32) · Status (OK / error / blacklist) · Path. Поиск и фильтры по формату/типу/статусу.
- **ПКМ по плагину**: Add/remove favorite · Rescan · Reveal in Explorer · Blacklist (не загружать) · Category.
- Сканирование — **в отдельном процессе**, чтобы сломанный плагин не ронял YW Studio.

### 9.3 Поддерживаемые форматы (Windows)
| Формат | v1 | Примечание |
|---|---|---|
| VST3 (64-bit) | ✔ | основной формат |
| VST2 (64-bit, .dll) | ✔ | через clean-room заголовки (риск: VST2 SDK закрыт) |
| CLAP (64-bit) | ✔ | открытый формат |
| 32-bit плагины | — | мост, v2 |
| AU | — | только macOS, при порте на Mac |

---

## 10. Настройки (Options)

### 10.1 Audio (F10 ▸ вкладка Audio)
**Устройство**: Device (ASIO / WASAPI / DirectSound) · Status (вход/выход, общая задержка) · Auto close · Sample rate (44.1/48/88.2/96 кГц) · Buffer length · Show ASIO panel · Triple buffer.
**Поток обработки**: Priority · Safe overloads · Underruns (счётчик).
**Playback tracking**: Source (Driver / Hybrid / Mixer), Offset.
**CPU**: Multithreaded generator processing · Multithreaded mixer processing · Smart disable · Align tick lengths.
**Mixer**: Resampling (Linear … 24-point sinc) · Metronome track · Preview track · Reset plugins on transport · Play truncated notes.

### 10.2 MIDI
Устройства входа/выхода, enable, controller type, порт, link note on velocity to, MIDI clock sync. [v2]

### 10.3 General
**Display**: GUI scaling · Pop-up/Toolbar scaling · Animations · Smoothing · Refresh rate · High visibility.
**Language**: язык (RU / EN), названия нот (English / Neo-Latin), диезы/бемоли.
**GUI Input**: Use both mouse buttons in step sequencer · Click-and-hold gestures · Scroll speed.
**Undo**: Max undo levels · Undo knob tweaks · Alternate undo mode.
**Misc**: Auto name channels · Auto zip empty channels · Auto select linked modules · Auto zoom in piano roll · Detach all new plugins · Hide plugin toolbar · Select first channel with notes on pattern change · Default template · Startup project (empty / template / last).
**System**: Associate .yws files · Check for updates.

### 10.4 File
Папки браузера (до 30 пользовательских) · Папки рендеров/записей · Backup (автосохранение каждые N минут, количество копий) · Папка шаблонов.

### 10.5 Project (F11)
Project info: название, автор, жанр, описание, теги. Project settings: тактовый размер, PPQ (96…960), длина паттерна по умолчанию, темп по умолчанию.

### 10.6 Theme
Тёмная тема YW (по умолчанию), акцентный цвет, контраст. [v2]

---

## 11. Сводка хоткеев (v1)

| Хоткей | Действие |
|---|---|
| Space | Play/Stop |
| L | Pattern/Song |
| R | Record |
| F5 / F6 / F7 / F8 / F9 | Playlist / Channel Rack / Piano Roll / Plugin Picker / Mixer |
| Alt+F8 | Browser |
| F10 | Settings |
| F11 | Project info |
| F12 | Закрыть все окна |
| F2 | Rename/color |
| F4 | Next empty pattern |
| Ctrl+O / Ctrl+S / Shift+Ctrl+S / Ctrl+N | Open / Save / Save as / Save new version |
| Ctrl+R / Shift+Ctrl+R | Export WAV / MP3 |
| Ctrl+Z / Ctrl+Alt+Z | Undo / Redo |
| Ctrl+X / C / V / B | Cut / Copy / Paste / Duplicate |
| Ctrl+A / Ctrl+D | Select all / Deselect |
| Ctrl+L | Route to free mixer track |
| Ctrl+M | Metronome |
| Ctrl+T | Typing keyboard |
| Ctrl+H | Panic |
| Ctrl+F | Browser find |
| P / B / D / T / C / E / Z / Y | Draw / Paint / Delete / Mute / Slice / Select / Zoom / Play selected |
| Alt+C / Alt+Del | Clone / Delete channel |
| PgUp / PgDn | Zoom |
| Shift+1…5 | Quick zoom |

---

## 12. Объём первой рабочей версии (v1)

1. Главное окно, главное меню (все пункты на месте; нереализованные — неактивны), тулбар.
2. Аудио: WASAPI + ASIO, выбор устройства, буфер, sample rate.
3. Plugin Manager: папки, сканирование в отдельном процессе, VST3 + VST2 + CLAP.
4. Plugin Picker и Plugin Host (окно плагина, пресеты, детальные настройки — базовые).
5. Channel Rack: каналы, step sequencer, swing, graph editor (velocity/pan/pitch), меню.
6. YW Sampler.
7. Piano Roll: инструменты Draw/Paint/Delete/Mute/Slice/Select/Zoom, свойства нот, event editor, снап, базовые tools.
8. Playlist: треки, паттерн- и аудиоклипы, клипы автоматизации, маркеры, лупы.
9. Mixer: треки, фейдер/пан/stereo/mute/solo, 10 FX-слотов, посылы, sidechain, PDC.
10. Browser: дерево папок, поиск, предпрослушка, drag&drop.
11. Проект .yws: сохранение/открытие, автобэкап, undo/redo.
12. Экспорт WAV и MIDI, импорт MIDI.
13. Настройки: Audio, General (основное), File, Project.

---

*Источник: официальный онлайн-мануал FL Studio, Image-Line — разделы Menu bar & panels, File/Edit/Add/Patterns/View/Options/Tools/Help, Channel Rack, Piano roll, Playlist, Mixer, Browser, Plugin Wrapper, Audio/General settings. Документ описывает функциональность и раскладку; графика и тексты интерфейса YW Studio создаются независимо.*
