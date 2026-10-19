# Original attribute icons

`ui_icon_art.h` contains the four original attribute drawings from MENU.swf's character stats panel, sprite647. The stat text panel622 names their corresponding attributes, and the original placements identify the shapes unambiguously:

| Runtime icon | Source shape | Original panel position in twips |
|---|---:|---:|
| Strength |367| -2263,667 |
| Defense |366| -2267,2067 |
| Magic |364| 759,696 |
| Agility |365| 706,2047 |

AMENU is the arena selection display and its symbols are flags, not these attribute icons. No substitute icons were invented. Source shapes are cropped, proportionally fitted inside14x14 pixels, and centered in16x16 transparent cells. Their shared15-color source-derived RGB555 palette and four-bit indices total544 resident bytes. The bounded `ui_icon_decode(icon,out)` needs256 temporary bytes; no persistent image cache is necessary. Index0 is transparent. Keep original pale-gray/olive artwork legible against the chosen panel background.

Regenerate with `prepare_ui_icons.py`, `export_ui_icons.py` (local FFDec), and `convert_ui_icons.py`. Source dimensions, IDs and XML hash are in `generated/ui-icon-audit.json`. `tests/ui_icon_art_test.c` validates all four RGB555 source-reference hashes, index bounds, transparent decode guards and exact544-byte bank size. Preview: `build/ui-icons-preview.png`.
