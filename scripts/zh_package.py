"""Create a new, self-contained release. Never overwrite an existing/running release."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import re
import shutil
import subprocess
import sys
import uuid

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build_zh/src'
CRT = ROOT / '_toolchain/msvc/VC/Tools/MSVC/14.44.35207/redist/x64'
RUNTIME = ('vcruntime140.dll','vcruntime140_1.dll','vcruntime140_threads.dll',
           'msvcp140.dll','msvcp140_1.dll','msvcp140_2.dll','msvcp140_atomic_wait.dll',
           'msvcp140_codecvt_ids.dll','concrt140.dll')

def destination(name):
    if not re.fullmatch(r'ZH-Studio-[A-Za-z0-9_-]+', name):
        raise ValueError('Release name must be ZH-Studio- followed by letters, numbers, _ or -')
    output = ROOT / name
    if output.exists():
        raise FileExistsError(f'{output} already exists. Use a new release name; existing files will not be overwritten.')
    return output

def package(name):
    output = destination(name)
    required = [BUILD/'ZH-Studio.exe', BUILD/'BambuStudio.dll', ROOT/'LICENSE'] + [CRT/f for f in RUNTIME]
    for source in required:
        if not source.is_file(): raise FileNotFoundError(f'Missing build/runtime file: {source}')
    # A failed copy leaves an explicitly incomplete folder, never a valid-looking release.
    staging = ROOT / (name + '.incomplete-' + uuid.uuid4().hex[:8])
    staging.mkdir()
    try:
        shutil.copy2(BUILD/'ZH-Studio.exe', staging)
        for source in BUILD.glob('*.dll'): shutil.copy2(source, staging)
        for filename in RUNTIME: shutil.copy2(CRT/filename, staging)
        shutil.copy2(ROOT/'LICENSE', staging/'LICENSE')
        shutil.copytree(ROOT/'resources', staging/'resources')
        subprocess.run([sys.executable, str(ROOT/'tests/zh_release_test.py'), str(staging)], check=True)
        hashes = {path.name:hashlib.sha256(path.read_bytes()).hexdigest()
                  for path in staging.iterdir() if path.suffix.lower() in ('.exe','.dll')}
        manifest = {'product':'ZH-Studio','edition':'Alpha',
                    'packaged_at_utc':datetime.now(timezone.utc).isoformat(), 'binary_sha256':hashes,
                    'verification':'x64 PE, exact build hashes, runtime imports, local resources, licence'}
        (staging/'ZH-Studio-build.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
        staging.rename(output)
    except Exception:
        print(f'Packaging stopped. Incomplete files retained at: {staging}', file=sys.stderr)
        raise
    print(f'Verified release ready: {output}')
    return output

if __name__ == '__main__':
    try:
        package(sys.argv[1] if len(sys.argv)>1 else 'ZH-Studio-Refresh')
    except Exception as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
