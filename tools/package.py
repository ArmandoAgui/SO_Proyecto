#!/usr/bin/env python3
"""Empaqueta fuentes y PDF; excluye Git, cachés, binarios y la guía del docente."""
import pathlib
import zipfile

root = pathlib.Path(__file__).resolve().parents[1]
destination = root / 'build/entrega/proyecto10-dsm.zip'
with zipfile.ZipFile(destination, 'w', zipfile.ZIP_DEFLATED) as archive:
    for path in sorted(root.rglob('*')):
        relative = path.relative_to(root)
        if any(part in {'.git', 'build', '__pycache__', '.vscode', '.idea'} for part in relative.parts):
            continue
        if path.is_file() and path.suffix not in {'.pyc', '.log', '.aux', '.out', '.toc'}:
            archive.write(path, 'proyecto10-dsm/' + str(relative))
    for name in ('paper.pdf', 'presentacion.pdf'):
        archive.write(root / 'build/entrega' / name, 'proyecto10-dsm/entregables/' + name)
print(destination)
