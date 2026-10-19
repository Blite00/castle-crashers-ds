#!/usr/bin/env python3
"""Cross-platform host test runner: runs the checks listed in check.ps1.

check.ps1 stays the single list of checks (Windows users can keep running it
with the portable MSYS2 toolchain). This script reads it and runs the same
compiler invocations, test programs and Python checks on any OS with a host
C compiler (gcc/clang; set CC to choose one).

  python tools/check.py            # everything
  python tools/check.py --list     # print the commands without running them
Run tools/setup_assets.py first: the tests use the converted generated/ assets.
"""
import argparse, glob, hashlib, os, re, shutil, subprocess, sys
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import host_cc


def tokens(line, vars):
    out, i, n = [], 0, len(line)
    while i < n:
        c = line[i]
        if c.isspace():
            i += 1; continue
        tok = ''; splat = None
        while i < n and not line[i].isspace():
            c = line[i]
            if c == "'":
                j = line.index("'", i + 1); tok += line[i + 1:j]; i = j + 1
            elif c == '"':
                j = i + 1
                while line[j] != '"':
                    j += 1 + (line[j] == '`')
                tok += interp(line[i + 1:j], vars); i = j + 1
            elif c == '@' and not tok and re.match(r'@\w+', line[i:]):
                m = re.match(r'@(\w+)', line[i:]); splat = vars[m[1]]; i += len(m[0])
            elif c == '$':
                m = re.match(r'\$\(\$(\w+)\)|\$(\w+)', line[i:]); tok += str(vars[m[1] or m[2]]); i += len(m[0])
            else:
                tok += c; i += 1
        if splat is not None:
            out.extend(splat)
        else:
            # Windows programs read \" as a literal quote; pass the quote itself on every OS.
            out.append(tok.replace('\\"', '"'))
    return out


def interp(s, vars):
    return re.sub(r'\$\(\$(\w+)\)|\$(\w+)', lambda m: str(vars[m[1] or m[2]]), s)


def parse(lines):
    """Return a nested list of statements; loops become ('for', var, values, body)."""
    stmts, stack = [], []
    cur = stmts
    for raw in lines:
        line = raw.strip()
        m = re.match(r"foreach \(\$(\w+) in (?:@\((.*)\)|(\d+)\.\.(\d+))\) \{$", line)
        if m:
            vals = re.findall(r"'([^']*)'", m[2]) if m[2] is not None else list(range(int(m[3]), int(m[4]) + 1))
            body = []
            cur.append(('for', m[1], vals, body)); stack.append(cur); cur = body
        elif line == '}':
            cur = stack.pop()
        elif line:
            cur.append(line)
    return stmts


class Runner:
    def __init__(self, cc, dry):
        self.cc, self.dry, self.rc, self.count = str(cc), dry, 0, 0
        self.skip_until = None

    def run(self, cmd):
        if self.dry:
            print(' '.join(cmd)); self.rc = 0; return
        exe = cmd[0]
        if exe.startswith('./'):
            cmd = [str(ROOT / exe)] + cmd[1:]
        self.rc = subprocess.call(cmd, cwd=str(ROOT))
        self.count += 1

    def exec(self, stmts, vars):
        for s in stmts:
            if self.skip_until:
                if isinstance(s, str) and self.skip_until in s:
                    self.skip_until = None
                else:
                    continue
            if isinstance(s, tuple):
                _, var, vals, body = s
                for v in vals:
                    self.exec(body, dict(vars, **{var: v}))
                continue
            if s.startswith(('$ErrorActionPreference', 'Push-Location', '$savedPath', 'try {', '$cc =', '$env:PATH', '} finally')):
                continue
            m = re.match(r"if \(\$LASTEXITCODE -ne 0\) \{ throw (['\"])(.*)\1 \}$", s)
            if m:
                if self.rc != 0:
                    sys.exit('FAILED: ' + interp(m[2], vars))
                continue
            m = re.match(r"\$(\w+) = @\(\)$", s)
            if m:
                vars[m[1]] = []; continue
            m = re.match(r"if \(\$(\w+) -eq '([^']*)'\) \{ \$(\w+) \+= '([^']*)' \}$", s)
            if m:
                if str(vars[m[1]]) == m[2]:
                    vars[m[3]] = vars[m[3]] + [m[4]]
                continue
            m = re.match(r"New-Item -ItemType Directory -Force (\S+) \| Out-Null$", s)
            if m:
                (ROOT / m[1]).mkdir(parents=True, exist_ok=True); continue
            m = re.match(r"Copy-Item (\S+) (\S+) -Force$", s)
            if m:
                for f in glob.glob(str(ROOT / m[1])):
                    shutil.copy(f, ROOT / m[2])
                continue
            m = re.match(r"if \(\(Get-FileHash (\S+)\)\.Hash -ne \(Get-FileHash (\S+)\)\.Hash\) \{ throw '(.*)' \}$", s)
            if m:
                if not self.dry and (ROOT / m[1]).read_bytes() != (ROOT / m[2]).read_bytes():
                    sys.exit('FAILED: ' + m[3])
                continue
            m = re.match(r"Write-Output '(.*)'$", s)
            if m:
                print(m[1]); continue
            if s.startswith('& '):
                if s.startswith('& $cc '):
                    t = [self.cc] + tokens(s[len('& $cc '):], vars)
                else:
                    t = tokens(s[2:], vars)
                self.run(t); continue
            if s.startswith('python '):
                self.run([sys.executable] + tokens(s[7:], vars)); continue
            sys.exit('check.py does not understand this check.ps1 line:\n  ' + s)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--list', action='store_true')
    ap.add_argument('--start', metavar='TEXT', help='skip the checks before the first check.ps1 line containing TEXT')
    a = ap.parse_args()
    if not (ROOT / 'generated' / 'castle_art.h').is_file():
        sys.exit('generated/ is missing: run  python tools/setup_assets.py --no-build  first.')
    (ROOT / 'build').mkdir(exist_ok=True)
    cfg = ROOT / '.tools' / 'setup.json'
    if cfg.is_file():  # written by setup_assets.py; some audio checks re-read the game's sound files
        import json
        c = json.loads(cfg.read_text())
        os.environ.setdefault('CC_GAME_DIR', c['game']); os.environ.setdefault('FFDEC_JAR', c['ffdec'])
        os.environ.setdefault('FFMPEG', c['ffmpeg'])
    r = Runner(host_cc.path(ROOT), a.list)
    r.skip_until = a.start
    r.exec(parse((ROOT / 'check.ps1').read_text(encoding='utf-8').splitlines()), {})
    if not a.list:
        print(f'All host checks passed ({r.count} steps).')


if __name__ == '__main__':
    main()
