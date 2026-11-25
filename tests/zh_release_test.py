"""Read-only verification of the packaged ZH-Studio Windows release."""
from pathlib import Path
import hashlib
import re
import subprocess
import struct
import os
import sys

ROOT = Path(__file__).resolve().parents[1]
RELEASE = Path(sys.argv[1]).resolve() if len(sys.argv)>1 else ROOT / 'ZH-Studio-Refresh'
assert RELEASE.parent == ROOT, 'Release must be inside the project'
BUILD = ROOT / 'build_zh/src'
DUMPBIN = ROOT / '_toolchain/msvc/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/dumpbin.exe'
for name in ('ZH-Studio.exe', 'BambuStudio.dll'):
    packaged = (RELEASE/name).read_bytes()
    compiled = (BUILD/name).read_bytes()
    assert hashlib.sha256(packaged).digest() == hashlib.sha256(compiled).digest(), f'{name}: stale package'
    pe = struct.unpack_from('<I',packaged,0x3c)[0]
    assert packaged[pe:pe+4] == b'PE\0\0'
    assert struct.unpack_from('<H',packaged,pe+4)[0] == 0x8664, f'{name}: not x64'
    print(f'{name}: final build hash matches; x64 PE validated; {len(packaged):,} bytes')
assert b' - BambuStudio' not in (RELEASE/'BambuStudio.dll').read_bytes(), 'old window title'
assert (RELEASE/'LICENSE').read_bytes() == (ROOT/'LICENSE').read_bytes(), 'upstream licence must stay unchanged'
for path in ('resources/images/ZH-Studio.svg','resources/web/homepage3/home.html',
             'resources/web/homepage3/js/zh-studio.js','resources/web/homepage3/css/zh-studio.css',
             'resources/i18n/de/BambuStudio.mo','resources/profiles'):
    assert (RELEASE/path).exists(), f'Missing {path}'
    if (ROOT/path).is_file():
        assert hashlib.sha256((RELEASE/path).read_bytes()).digest() == hashlib.sha256((ROOT/path).read_bytes()).digest(), f'Stale packaged resource: {path}'

system = Path(os.environ['SystemRoot'])/'System32'
files = {p.name.lower():p for p in RELEASE.iterdir() if p.suffix.lower() in ('.exe','.dll')}
missing = set()
for name,path in files.items():
    result = subprocess.run([str(DUMPBIN),'/nologo','/dependents',str(path)],capture_output=True)
    assert result.returncode == 0, f'dumpbin failed for {name}'
    deps = re.findall(r'^\s+([A-Za-z0-9_.-]+\.dll)\s*$',result.stdout.decode('utf-8',errors='replace'),flags=re.M|re.I)
    for dep in deps:
        lower = dep.lower()
        if lower.startswith(('api-ms-','ext-ms-')): continue
        if lower not in files and not (system/dep).exists(): missing.add(dep)
assert not missing, f'Unresolved runtime dependencies: {sorted(missing)}'
print(f'Runtime import dependencies checked for {len(files)} packaged binaries.')
print('Final release validation passed. Printer/cloud connectivity is not tested by this check.')
