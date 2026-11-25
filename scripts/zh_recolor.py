"""Recolour only ZH-Studio's UI accents; never printer/filament profiles or 3MF metadata."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
GUI = ROOT / 'src/slic3r/GUI'
COLORS = {'00ae42':'0EA5E9','00af42':'0EA5E9','21a452':'0EA5E9','3dcb73':'38BDF8',
          '1b8844':'0284C7','1c8a46':'0284C7','37b865':'38BDF8','edfaf2':'E0F2FE'}
changed = 0
for path in GUI.rglob('*'):
    if path.suffix not in ('.cpp', '.hpp', '.h'): continue
    if path.name in ('ZHTheme.hpp', 'BitmapCache.cpp'): continue
    s = path.read_text(encoding='utf-8-sig')
    original = s
    # Redirect literal wxColour accent constructors into the palette, including headers.
    for rgb, func in [('0,174,66','accent'), ('27,136,68','down'), ('61,203,115','hovered')]:
        args = r'\s*,\s*'.join(rgb.split(','))
        s = re.sub(r'wxColou?r\(\s*'+args+r'\s*\)', 'ZHTheme::'+func+'()', s)
    funcs = {'00ae42':'accent','00af42':'accent','3dcb73':'hovered','1b8844':'down','edfaf2':'tint'}
    for value, func in funcs.items():
        s = re.sub(r'wxColou?r\(\s*"#'+value+r'"\s*\)', 'ZHTheme::'+func+'()', s, flags=re.I)
    # Normalise remaining hex UI constants to the canonical cyan palette.
    for value, new in COLORS.items():
        s = re.sub('#'+value, '#'+new, s, flags=re.I)
    if s != original:
        if 'ZHTheme::' in s:
            include = Path(*(['..'] * len(path.parent.relative_to(GUI).parts)), 'ZHTheme.hpp').as_posix()
            if f'#include "{include}"' not in s:
                s = f'#include "{include}"\n'+s
        path.write_text(s, encoding='utf-8')
        changed += 1
# Recolour SVG UI icons; skip factual printer and filament artwork.
for folder in (ROOT / 'resources/images', ROOT / 'resources/web'):
    for path in folder.rglob('*'):
        if path.suffix.lower() not in ('.svg', '.css', '.html'): continue
        if any(x in path.name.lower() for x in ('printer_thumbnail', 'filament_swatch')): continue
        s = path.read_text(encoding='utf-8-sig')
        original = s
        for value, new in COLORS.items():
            s = re.sub('#'+value, '#'+new, s, flags=re.I)
        s = re.sub(r'(rgba?)\(\s*0\s*,\s*174\s*,\s*66\b', lambda m: m.group(1)+'(14,165,233', s)
        if s != original:
            path.write_text(s, encoding='utf-8')
            changed += 1
print(f'Recoloured {changed} UI source / resource files. Profiles and material colours are untouched.')
