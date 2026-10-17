#!/usr/bin/env python3
"""Demo comprobada: guarda trazas reales, configuración y huella del código."""
import datetime
import hashlib
import json
import pathlib
import platform
import subprocess
import sys
from zoneinfo import ZoneInfo

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/integration'))
from test_dsm import DSMTest  # noqa: E402


def main():
    destination = ROOT / 'docs/testing/evidence'
    destination.mkdir(parents=True, exist_ok=True)
    case = DSMTest()
    case.setUp()
    nodes = {}
    commands = [
        (0, 'write 3 hola desde N0', 'RESULT WRITE'),
        (1, 'read 3', 'data="hola desde N0"'),
        (0, 'status 3', 'state=INVALID'),
        (1, 'status 3', 'state=OWNER'),
        (0, 'read-copy 3', 'RESULT READ_COPY'),
        (0, 'status 3', 'state=READ_ONLY'),
        (1, 'write 3 actualizado por N1', 'RESULT WRITE'),
        (0, 'status 3', 'state=INVALID'),
        (0, 'read 3', 'data="actualizado por N1"'),
        (0, 'status 3', 'state=OWNER'),
        (1, 'status 3', 'state=INVALID'),
    ]
    try:
        nodes = case.pair(1)
        for node_id, command, expected in commands:
            case.command(nodes, node_id, command, expected)
            print(f'N{node_id}> {command}')
        case.close_pair(nodes)
        digest = hashlib.sha256()
        sources = [ROOT / 'Makefile'] + sorted((ROOT / 'src').rglob('*.c')) + sorted((ROOT / 'include').rglob('*.h'))
        for source in sources:
            digest.update(str(source.relative_to(ROOT)).encode())
            digest.update(source.read_bytes())
        report = {
            'date_local': datetime.datetime.now(ZoneInfo('America/El_Salvador')).isoformat(),
            'os': platform.platform(),
            'compiler': subprocess.check_output(['cc', '--version'], text=True).splitlines()[0],
            'base_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
            'working_tree_dirty': bool(subprocess.check_output(['git', 'status', '--porcelain'], cwd=ROOT)),
            'source_sha256': digest.hexdigest(),
            'result': 'PASS',
            'commands': [{'node': node, 'command': command} for node, command, _ in commands],
            'note': 'Puertos efímeros; misma lógica que la demo manual 5000/5001. No mide latencia.',
        }
        for node_id, node in nodes.items():
            output = case.output(node)
            (destination / f'demo-node{node_id}.txt').write_text(output)
            print(output)
        (destination / 'demo-summary.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n')
        print('PASS: evidencia guardada en docs/testing/evidence/')
    finally:
        case.tearDown()


if __name__ == '__main__':
    main()
