"""Generate ZH-Studio identity assets. Run with Pillow; no system installation needed."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "resources/images"
INK = "#0B1220"
CYAN = "#0EA5E9"
# One continuous Z followed by an H, inspired by a toolpath on a build plate.
POLYGONS = [
    [(38, 64), (122, 64), (122, 83), (66, 164), (122, 164), (122, 187), (34, 187), (34, 168), (91, 87), (38, 87)],
    [(142, 64), (165, 64), (165, 111), (197, 111), (197, 64), (220, 64), (220, 187), (197, 187), (197, 135), (165, 135), (165, 187), (142, 187)],
]

def icon_svg(background=True, ink=False):
    bg = '<rect x="4" y="4" width="248" height="248" rx="54" fill="#0B1220"/>' if background else ''
    paths = ''.join('<polygon points="' + ' '.join(f'{x},{y}' for x,y in p) + '" fill="' + (INK if ink else CYAN) + '"/>' for p in POLYGONS)
    return f'<svg xmlns="http://www.w3.org/2000/svg" width="256" height="256" viewBox="0 0 256 256">{bg}{paths}<path d="M38 210H220" stroke="#22D3EE" stroke-width="4" stroke-linecap="round"/></svg>'

def icon(size, background=True):
    image = Image.new('RGBA', (1024, 1024), (0,0,0,0))
    draw = ImageDraw.Draw(image)
    if background:
        draw.rounded_rectangle((16,16,1008,1008), radius=216, fill=INK)
    for p in POLYGONS:
        draw.polygon([(x*4,y*4) for x,y in p], fill=CYAN)
    draw.line((152,840,880,840), fill='#22D3EE', width=16)
    return image.resize((size,size), Image.Resampling.LANCZOS)

OUT.joinpath('ZH-Studio.svg').write_text(icon_svg(), encoding='utf-8')
# Legacy filenames remain aliases because hundreds of upstream dialogs load them.
for name in ('BambuStudio.svg', 'topbar_logo.svg'):
    OUT.joinpath(name).write_text(icon_svg(), encoding='utf-8')
OUT.joinpath('BambuStudioBlack.svg').write_text(icon_svg(False, True), encoding='utf-8')
OUT.joinpath('splash_logo.svg').write_text(icon_svg(), encoding='utf-8')
for size in (32, 48, 64, 128, 192, 256, 512):
    icon(size).save(OUT / f'ZH-Studio_{size}px.png')
    icon(size).save(OUT / f'BambuStudio_{size}px.png')
icon(256).save(OUT / 'BambuStudio.png')
icon(192, False).save(OUT / 'BambuStudio_192px_transparent.png')
icon(192).convert('LA').convert('RGBA').save(OUT / 'BambuStudio_192px_grayscale.png')
icon(128).save(OUT / 'BambuStudio-mac_128px.png')
for name in ('ZH-Studio.ico', 'BambuStudio.ico', 'BambuStudioTitle.ico', 'BambuStudio-mac_256px.ico'):
    icon(256).save(OUT / name, format='ICO', sizes=[(16,16),(24,24),(32,32),(48,48),(64,64),(128,128),(256,256)])
for path in (OUT / 'BambuStudio.icns', ROOT / 'resources/Icon.icns'):
    icon(1024).save(path, format='ICNS')
about = '''<svg xmlns="http://www.w3.org/2000/svg" width="560" height="250" viewBox="0 0 560 250">
<rect width="560" height="250" fill="#0B1220"/>
<path d="M0 0H560M0 249H560" stroke="#0EA5E9" stroke-width="3"/>
<g transform="translate(30,34) scale(.52)">MONOGRAM</g>
<text x="192" y="112" fill="#F1F5F9" font-family="Segoe UI,Arial,sans-serif" font-weight="700" font-size="44">ZH-Studio</text>
<text x="194" y="147" fill="#7DD3FC" font-family="Segoe UI,Arial,sans-serif" font-size="16" letter-spacing="3">MAKE IT YOURS.</text>
</svg>'''.replace('MONOGRAM', icon_svg(False).split('>',1)[1].rsplit('</svg>',1)[0])
OUT.joinpath('BambuStudio_about.svg').write_text(about, encoding='utf-8')
# A preview of the brand for local visual verification.
image = Image.new('RGB',(960,540),INK)
draw = ImageDraw.Draw(image)
for x in range(0,960,40): draw.line((x,0,x,540), fill='#111F32')
for y in range(0,540,40): draw.line((0,y,960,y), fill='#111F32')
image.paste(icon(220), (110,130), icon(220))
fontfile = 'C:/Windows/Fonts/segoeuib.ttf'
font = ImageFont.truetype(fontfile,70)
small = ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',24)
draw.text((372,162),'ZH-Studio',fill='#F1F5F9',font=font)
draw.text((378,266),'MAKE IT YOURS.',fill='#7DD3FC',font=small)
draw.text((112,430),'SLICING  /  PRINTING  /  YOUR WORKSPACE',fill='#64748B',font=small)
image.save(OUT / 'ZH-Studio-brand-preview.png')
print('ZH-Studio logos, PNG, ICO, ICNS and about artwork generated.')
