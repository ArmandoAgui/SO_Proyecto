#!/usr/bin/env python3
"""Fase 1: dos procesos C reales; solo biblioteca estándar de Python."""
import pathlib
import os
import signal
import socket
import subprocess
import tempfile
import time
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
BINARY = pathlib.Path(os.environ.get('DSM_BINARY', str(ROOT / 'build/dsm-node')))


class NodeHarness(unittest.TestCase):
    def setUp(self):
        self.nodes = []

    def tearDown(self):
        for process, log in self.nodes:
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=3)
            process.stdin.close()
            log.close()

    def ports(self):
        with socket.socket() as first, socket.socket() as second:
            first.bind(('127.0.0.1', 0))
            second.bind(('127.0.0.1', 0))
            return first.getsockname()[1], second.getsockname()[1]

    def start(self, node_id, local_port, peer_port):
        log = tempfile.TemporaryFile(mode='w+b')
        process = subprocess.Popen(
            [str(BINARY), '--id', str(node_id), '--port', str(local_port),
             '--peer-port', str(peer_port)], stdin=subprocess.PIPE,
            stdout=log, stderr=subprocess.STDOUT)
        node = (process, log)
        self.nodes.append(node)
        return node

    def output(self, node):
        # pread leaves the child's shared stdout file offset unchanged.
        return os.pread(node[1].fileno(), 100000, 0).decode(errors='replace')

    def wait_log(self, node, expected, timeout=5):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            output = self.output(node)
            if expected in output:
                return output
            if node[0].poll() is not None:
                self.fail(f'Proceso terminó antes de {expected!r}:\n{output}')
            time.sleep(0.02)
        self.fail(f'Plazo excedido esperando {expected!r}:\n{self.output(node)}')

    def pair(self, first_id, exact_ports=False):
        p0, p1 = (5000, 5001) if exact_ports else self.ports()
        nodes = {}
        nodes[first_id] = self.start(first_id, (p0, p1)[first_id], (p1, p0)[first_id])
        self.wait_log(nodes[first_id], 'Escuchando')
        if first_id == 0:
            self.wait_log(nodes[0], 'Reintentando')
        second_id = 1 - first_id
        nodes[second_id] = self.start(second_id, (p0, p1)[second_id], (p1, p0)[second_id])
        for node in nodes.values():
            self.wait_log(node, 'DSM_READY')
            self.assertIsNone(node[0].poll())
        self.assertIn('Mensaje recibido: HELLO 0', self.output(nodes[1]))
        self.assertIn('Mensaje recibido: HELLO_ACK 1', self.output(nodes[0]))
        self.assertEqual(self.output(nodes[1]).count('Conexión recibida y aceptada'), 1)
        return nodes

    def finish(self, node, expected=0):
        self.assertEqual(node[0].wait(timeout=5), expected, self.output(node))
        self.assertIn('Socket servidor cerrado', self.output(node))



class SocketsTest(NodeHarness):
    def test_help(self):
        result = subprocess.run([str(BINARY), '--help'], capture_output=True, text=True, timeout=3)
        self.assertEqual(result.returncode, 0)
        self.assertIn('--peer-port', result.stdout)

    def test_invalid_arguments(self):
        cases = [[], ['--id'], ['--id', '2'], ['--unknown'],
                 ['--id', '-1'], ['--id', '0', '--port', '0'],
                 ['--port', '65536'], ['--port', 'abc'], ['--port', '-2'],
                 ['--port', '999999999999999999999999'], ['--port', '12x'],
                 ['--port', '5000', '--port', '5001'],
                 ['--id', '0', '--port', '5000', '--peer-port', '5000']]
        for args in cases:
            with self.subTest(args=args):
                result = subprocess.run([str(BINARY), *args], capture_output=True, text=True, timeout=3)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn('Error:', result.stderr)

    def test_node0_first_exact_commands_exit(self):
        nodes = self.pair(0, exact_ports=True)
        nodes[0][0].stdin.write(b'exit\n')
        nodes[0][0].stdin.flush()
        self.finish(nodes[0])
        self.finish(nodes[1])
        self.assertIn('Peer desconectado', self.output(nodes[1]))

    def test_node1_first_enter(self):
        nodes = self.pair(1)
        nodes[1][0].stdin.write(b'\n')
        nodes[1][0].stdin.flush()
        self.finish(nodes[1])
        self.finish(nodes[0])

    def test_signal_and_peer_close(self):
        nodes = self.pair(1)
        nodes[0][0].send_signal(signal.SIGINT)
        self.finish(nodes[0])
        self.finish(nodes[1])
        self.assertIn('Señal de cierre', self.output(nodes[0]))

    def test_eof(self):
        nodes = self.pair(1)
        nodes[0][0].stdin.close()
        self.finish(nodes[0])
        self.finish(nodes[1])

    def test_occupied_port(self):
        with socket.socket() as occupied:
            occupied.bind(('127.0.0.1', 0))
            occupied.listen()
            local_port = occupied.getsockname()[1]
            peer_port = 1 if local_port != 1 else 2
            node = self.start(1, local_port, peer_port)
            self.assertEqual(node[0].wait(timeout=3), 1)
            self.assertIn('bind/listen', self.output(node))

    def test_cancel_during_retry(self):
        local, peer = self.ports()
        node = self.start(0, local, peer)
        self.wait_log(node, 'Reintentando')
        node[0].terminate()
        self.finish(node)

    def test_cancel_during_accept(self):
        local, peer = self.ports()
        node = self.start(1, local, peer)
        self.wait_log(node, 'Esperando conexión')
        node[0].terminate()
        self.finish(node)

    def test_fragmented_hello_and_premature_eof(self):
        local, peer = self.ports()
        node = self.start(1, local, peer)
        self.wait_log(node, 'Escuchando')
        with socket.create_connection(('127.0.0.1', local), timeout=3) as client:
            client.sendall(b'HEL')
            self.wait_log(node, 'Conexión recibida')
            self.assertNotIn('Mensaje recibido', self.output(node))
            client.sendall(b'LO 0\n')
            received = b''
            while not received.endswith(b'\n'):
                chunk = client.recv(1)
                self.assertTrue(chunk)
                received += chunk
            self.assertEqual(received, b'HELLO_ACK 1\n')
            self.wait_log(node, 'Intercambio HELLO completo')
            # Drain the new binary config before deliberately truncating a frame.
            remaining = 52
            while remaining:
                chunk = client.recv(remaining)
                self.assertTrue(chunk)
                remaining -= len(chunk)
            client.sendall(b'partial')
            client.shutdown(socket.SHUT_WR)
            self.finish(node, expected=1)
        self.assertIn('Error al recibir mensaje', self.output(node))

    def test_hello_timeout(self):
        local, peer = self.ports()
        node = self.start(1, local, peer)
        self.wait_log(node, 'Escuchando')
        with socket.create_connection(('127.0.0.1', local), timeout=3):
            self.assertEqual(node[0].wait(timeout=13), 1, self.output(node))
        self.assertIn('Error al esperar HELLO', self.output(node))
        self.assertIn('Socket servidor cerrado', self.output(node))

    def test_retry_exhaustion(self):
        local, peer = self.ports()
        node = self.start(0, local, peer)
        self.assertEqual(node[0].wait(timeout=35), 1, self.output(node))
        output = self.output(node)
        self.assertEqual(output.count('Intentando conectar'), 30)
        self.assertIn('Error al conectar con peer', output)
        self.assertIn('Socket servidor cerrado', output)

    def test_invalid_hello(self):
        local, peer = self.ports()
        node = self.start(1, local, peer)
        self.wait_log(node, 'Escuchando')
        with socket.create_connection(('127.0.0.1', local), timeout=3) as client:
            client.sendall(b'HELLO 1\n')
            self.finish(node, expected=1)
        self.assertIn('Error al validar mensaje', self.output(node))


if __name__ == '__main__':
    unittest.main(verbosity=2)
