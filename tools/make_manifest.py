#!/usr/bin/env python3
from pathlib import Path
import hashlib
ROOT = Path(__file__).resolve().parents[1]
SKIP_DIRS = {'.git', 'build', 'build-run', 'dist', '.DS_Store'}
items = []
for p in sorted(ROOT.rglob('*')):
    if not p.is_file():
        continue
    rel = p.relative_to(ROOT).as_posix()
    parts = set(p.relative_to(ROOT).parts)
    if parts & SKIP_DIRS:
        continue
    if rel == 'MANIFEST.sha256':
        continue
    h = hashlib.sha256(p.read_bytes()).hexdigest()
    items.append(f"{h}  {rel}\n")
(ROOT / 'MANIFEST.sha256').write_text(''.join(items))
print(f'manifest: {len(items)} files')
