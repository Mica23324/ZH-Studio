"""Narrow string literals with non-ASCII text become mojibake in wxString on Windows.

Every user-visible literal in the ZH-Studio GUI additions must go through text() or
wxString::FromUTF8(). This check fails the build suite if a new unwrapped literal slips in.
"""
from pathlib import Path
import re
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCES = [
    'src/slic3r/GUI/ZHStudioTools.cpp',
    'src/slic3r/GUI/ZHStudioTools.hpp',
    'src/slic3r/GUI/ZHTheme.hpp',
    'src/slic3r/GUI/ZHCostModel.hpp',
    'src/slic3r/GUI/ZHSplash.cpp',
]
LITERAL = re.compile(r'"(?:[^"\\]|\\.)*"')
ALLOWED_CALLS = {'text', 'FromUTF8'}


def strip_comments(source):
    out = []
    i = 0
    n = len(source)
    while i < n:
        if source.startswith('//', i):
            j = source.find('\n', i)
            i = n if j < 0 else j
        elif source.startswith('/*', i):
            j = source.find('*/', i + 2)
            i = n if j < 0 else j + 2
            out.append(' ')
        elif source[i] == '"':
            m = LITERAL.match(source, i)
            out.append(m.group(0))
            i = m.end()
        elif source[i] == "'":
            j = i + 1
            while j < n and source[j] != "'":
                j += 2 if source[j] == '\\' else 1
            out.append(source[i:j + 1])
            i = j + 1
        else:
            out.append(source[i])
            i += 1
    return ''.join(out)


def enclosing_call(source, quote_index):
    depth = 0
    i = quote_index - 1
    while i >= 0:
        c = source[i]
        if c == '"':
            j = i - 1
            while j >= 0:
                if source[j] == '"' and source[j - 1] != '\\':
                    break
                j -= 1
            i = j - 1
            continue
        if c == ')':
            depth += 1
        elif c == '(':
            if depth == 0:
                j = i - 1
                while j >= 0 and (source[j].isalnum() or source[j] in '_:'):
                    j -= 1
                return source[j + 1:i].split('::')[-1]
            depth -= 1
        i -= 1
    return ''


class EncodingSafety(unittest.TestCase):
    def test_no_unwrapped_non_ascii_literals(self):
        offenders = []
        for name in SOURCES:
            source = strip_comments((ROOT / name).read_text(encoding='utf-8'))
            for m in LITERAL.finditer(source):
                literal = m.group(0)
                if not any(ord(c) > 127 for c in literal):
                    continue
                if enclosing_call(source, m.start()) in ALLOWED_CALLS:
                    continue
                line = source.count('\n', 0, m.start()) + 1
                offenders.append(f'{name}:{line}: {literal[:60]}')
        self.assertEqual(offenders, [], 'wrap these literals in text() or wxString::FromUTF8():\n' + '\n'.join(offenders))


if __name__ == '__main__':
    unittest.main()