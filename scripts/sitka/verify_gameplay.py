#!/usr/bin/env python3
"""Run Sitka's deterministic rules and optional real module/GLES checks locally."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
TESTS = ROOT / 'app/src/test/native'
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--render', action='store_true', help='also test actual module Update and Mesa stereo rendering (GLES2/EGL development libraries required)')
args = parser.parse_args()
flags = ['-std=c++17', '-Wall', '-Wextra', '-Werror', '-Wpedantic',
         '-I' + str(ROOT / 'app/src/main/jni/native')]
flags += shlex.split(os.environ.get('CPPFLAGS', ''))
flags += shlex.split(os.environ.get('CXXFLAGS', ''))
compiler = shlex.split(os.environ.get('CXX', 'g++'))
checks = [p for p in sorted(TESTS.glob('sitka_*_test.cpp'))
          if 'egl' not in p.stem and 'starter_loop' not in p.stem]
if args.render:
    checks += [TESTS / (name + '.cpp') for name in
               ['sitka_starter_loop_test', 'sitka_starter_loop_egl_test', 'sitka_terrain_egl_test']]
with tempfile.TemporaryDirectory(prefix='sitka-gameplay-') as tmp:
    for test in checks:
        target = str(Path(tmp) / test.stem)
        links = []
        if 'starter_loop' in test.stem or 'egl' in test.stem:
            links = shlex.split(os.environ.get('LDFLAGS', '')) + ['-lGLESv2', '-lz']
            if 'egl' in test.stem:
                links += ['-lEGL']
        subprocess.run(compiler + flags + [str(test), '-o', target] + links, check=True)
        subprocess.run([target], check=True)
        print('PASS ' + test.stem, flush=True)
print(f'PASS {len(checks)} Sitka checks; render={args.render}')
