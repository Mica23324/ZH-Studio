"""Syntax-check a native UI source using the real CMake flags, without writing into the build tree."""
from pathlib import Path
import subprocess
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build_zh'
source = sys.argv[1] if len(sys.argv)>1 else 'ZHStudioTools.cpp'
assert re.fullmatch(r'[A-Za-z0-9_]+\.cpp',source), 'Expected a GUI C++ filename'
relative = 'Widgets/'+source if source == 'StateColor.cpp' else source
target = 'src/slic3r/CMakeFiles/libslic3r_gui.dir/GUI/'+relative+'.obj'
commands = subprocess.check_output([str(ROOT/'_toolchain/ninja/ninja.exe'),'-C',str(BUILD),'-t','commands',target],text=True)
command = next(line for line in commands.splitlines() if source in line and 'cl.exe' in line)
# Keep /FI (the CMake pch header), but do not read/write compiled PCH or shared compiler databases.
command = re.sub(r' /(Yu|Yc|Fp|Fo|Fd)\S+', '', command)
command = re.sub(r' /(FS|MP|showIncludes)\b', '', command)
command += ' /Zs'
setup = ROOT/'_toolchain/msvc/setup_x64.bat'
result = subprocess.run(f'call "{setup}" && {command}',shell=True,cwd=BUILD,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
log = result.stdout.decode('utf-8',errors='replace')
(ROOT/'_toolchain'/f'zh_syntax_{source}.log').write_text(log,encoding='utf-8')
for line in log.splitlines():
    if 'error ' in line or 'fatal ' in line: print(line)
print(f'{source}: native syntax check exit={result.returncode}')
sys.exit(result.returncode)
