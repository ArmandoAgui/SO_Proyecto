#!/usr/bin/env python3
"""Escenarios del PDF y errores de protocolo con sockets TCP reales."""
import pathlib
import random
import socket
import struct
import unittest

from test_sockets import NodeHarness

MAGIC = 0x44534D31
NO_PAGE = 0xFFFFFFFF


def frame(kind, origin=0, request=0, page=NO_PAGE, owner=NO_PAGE, version=0, flags=0, payload=b''):
    return struct.pack('!10I', MAGIC, 1, kind, origin, request, page, owner,
                       version, flags, len(payload)) + payload


def receive_exact(connection, length):
    result = b''
    while len(result) < length:
        chunk = connection.recv(length - len(result))
        if not chunk:
            raise AssertionError('EOF antes de completar trama')
        result += chunk
    return result


class DSMTest(NodeHarness):
    def command(self, nodes, node_id, text, expected):
        node = nodes[node_id]
        before = len(self.output(node))
        node[0].stdin.write((text + '\n').encode())
        node[0].stdin.flush()
        import time
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            output = self.output(node)[before:]
            if expected in output:
                return output
            if node[0].poll() is not None:
                self.fail(self.output(node))
            time.sleep(0.01)
        self.fail(f'Esperaba {expected!r}:\n{self.output(node)}')

    def status(self, nodes, node_id, page):
        return self.command(nodes, node_id, f'status {page}', f'PAGE page={page}')

    def close_pair(self, nodes):
        nodes[0][0].stdin.write(b'exit\n')
        nodes[0][0].stdin.flush()
        for node in nodes.values():
            self.finish(node)

    def test_pdf_migration_and_invalidation(self):
        nodes = self.pair(1)
        self.command(nodes, 0, 'write 3 hola', 'data="hola"')
        self.command(nodes, 1, 'read 3', 'RESULT READ')
        self.assertIn('state=INVALID', self.status(nodes, 0, 3))
        self.assertIn('state=OWNER', self.status(nodes, 1, 3))
        self.assertIn('PAGE_FAULT', self.output(nodes[1]))
        self.command(nodes, 0, 'read-copy 3', 'RESULT READ_COPY')
        self.assertIn('state=READ_ONLY', self.status(nodes, 0, 3))
        self.command(nodes, 1, 'write 3 nuevo', 'data="nuevo"')
        self.assertIn('SEND INVALIDATE', self.output(nodes[1]))
        self.assertIn('state=INVALID', self.status(nodes, 0, 3))
        self.command(nodes, 0, 'read 3', 'data="nuevo"')
        self.assertIn('state=OWNER', self.status(nodes, 0, 3))
        self.assertIn('state=INVALID', self.status(nodes, 1, 3))
        self.close_pair(nodes)

    def test_write_waits_for_remote_reader_ack(self):
        nodes = self.pair(1)
        self.command(nodes, 0, 'write 0 original', 'RESULT WRITE')
        self.command(nodes, 1, 'read-copy 0', 'RESULT READ_COPY')
        self.assertIn('state=READ_ONLY', self.status(nodes, 1, 0))
        result = self.command(nodes, 0, 'write 0 updated', 'RESULT WRITE')
        self.assertLess(result.index('SEND INVALIDATE'), result.index('RECV ACK'))
        self.assertLess(result.index('RECV ACK'), result.index('GRANT_WRITE'))
        self.assertLess(result.index('GRANT_WRITE'), result.index('RESULT WRITE'))
        self.assertIn('state=INVALID', self.status(nodes, 1, 0))
        self.command(nodes, 1, 'read 0', 'data="updated"')
        self.close_pair(nodes)

    def test_remote_writer_copy_and_existing_owner(self):
        nodes = self.pair(1)
        self.command(nodes, 1, 'read-copy 255', 'data=""')
        self.command(nodes, 1, 'write 255 desde copia', 'data="desde copia"')
        self.command(nodes, 1, 'write 255 segunda', 'data="segunda"')
        self.command(nodes, 0, 'read-copy 255', 'data="segunda"')
        self.command(nodes, 0, 'write 255 desde N0', 'data="desde N0"')
        self.command(nodes, 1, 'read-copy 255', 'data="desde N0"')
        self.command(nodes, 1, 'read 255', 'data="desde N0"')
        self.assertIn('state=INVALID', self.status(nodes, 0, 255))
        self.close_pair(nodes)

    def test_cli_errors_and_maximum_page(self):
        nodes = self.pair(1)
        for text in ['read -1', 'read 256', 'write 0', 'unknown', 'read 1x', 'help extra',
                     'write 1 ' + 'x' * 4096, 'write 1 ' + 'x' * 5000]:
            self.command(nodes, 0, text, 'Error: comando inválido')
        self.assertIn('version=0', self.status(nodes, 0, 1))
        text = 'x' * 4095
        self.command(nodes, 1, 'write 255 ' + text, 'RESULT WRITE')
        received = self.command(nodes, 0, 'read 255', 'RESULT READ')
        self.assertIn('data="' + text + '"', received)
        self.close_pair(nodes)

    def test_randomized_sequential_model(self):
        nodes = self.pair(1)
        rng = random.Random(10)
        expected = {page: '' for page in range(4)}
        for i in range(48):
            node_id, page = rng.randrange(2), rng.randrange(4)
            operation = rng.choice(['write', 'read', 'read-copy'])
            if operation == 'write':
                expected[page] = f'value-{i}-node-{node_id}'
                command = f'write {page} {expected[page]}'
            else:
                command = f'{operation} {page}'
            output = self.command(nodes, node_id, command, 'RESULT ')
            self.assertIn('data="' + expected[page] + '"', output)
        self.close_pair(nodes)

    def fake_coordinator(self):
        local, peer = self.ports()
        node = self.start(1, local, peer)
        self.wait_log(node, 'Escuchando')
        connection = socket.create_connection(('127.0.0.1', local), timeout=3)
        self.addCleanup(connection.close)
        connection.sendall(b'HELLO 0\n')
        self.assertEqual(receive_exact(connection, 12), b'HELLO_ACK 1\n')
        header = receive_exact(connection, 40)
        self.assertEqual(struct.unpack('!10I', header)[2], 1)
        receive_exact(connection, 12)
        config = frame(1, payload=struct.pack('!3I', 0, 256, 4096))
        # Fragment the binary header/body across independent sends.
        for start in range(0, len(config), 3):
            connection.sendall(config[start:start + 3])
        self.wait_log(node, 'DSM_READY')
        return node, connection

    def test_bad_binary_frames(self):
        for bad in [frame(99, request=1, page=0),
                    frame(5, request=1, page=0, payload=b'x'),
                    frame(7, request=1, page=256),
                    struct.pack('!10I', MAGIC, 1, 5, 0, 1, 0, 0, 0, 0, 999999),
                    frame(6, origin=1, request=99, page=0, owner=1, flags=3)]:
            with self.subTest(frame=bad[:40]):
                node, connection = self.fake_coordinator()
                connection.sendall(bad)
                self.finish(node, expected=1)
                connection.close()

    def test_busy_with_delayed_invalidation_ack(self):
        local, peer = self.ports()
        with socket.socket() as server:
            server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server.bind(('127.0.0.1', peer))
            server.listen(1)
            server.settimeout(3)
            node = self.start(0, local, peer)
            connection, _ = server.accept()
        with connection:
            connection.settimeout(3)
            self.assertEqual(receive_exact(connection, 8), b'HELLO 0\n')
            connection.sendall(b'HELLO_ACK 1\n')
            receive_exact(connection, 52)
            connection.sendall(frame(1, origin=1, payload=struct.pack('!3I', 1, 256, 4096)))
            self.wait_log(node, 'DSM_READY')
            node[0].stdin.write(b'write 3 original\n'); node[0].stdin.flush()
            self.wait_log(node, 'RESULT WRITE req=1')
            connection.sendall(frame(2, origin=1, request=1, page=3, flags=2))
            header = struct.unpack('!10I', receive_exact(connection, 40))
            self.assertEqual(header[2], 5)
            receive_exact(connection, 4096)
            connection.sendall(frame(8, origin=1, request=1, page=3, owner=0, version=1, flags=2))
            self.wait_log(node, 'valid_mask=3')
            node[0].stdin.write(b'write 3 updated\n'); node[0].stdin.flush()
            invalidate = struct.unpack('!10I', receive_exact(connection, 40))
            self.assertEqual(invalidate[2], 7)
            self.assertNotIn('RESULT WRITE req=2', self.output(node))
            connection.sendall(frame(2, origin=1, request=2, page=4, flags=1))
            error = struct.unpack('!10I', receive_exact(connection, 40))
            self.assertEqual((error[2], error[4], error[8]), (9, 2, 1))
            self.assertNotIn('RESULT WRITE req=2', self.output(node))
            connection.sendall(frame(8, request=2, page=3, owner=0, version=1, flags=1))
            self.wait_log(node, 'RESULT WRITE req=2')
            self.assertIn('data="updated"', self.output(node))
            node[0].stdin.write(b'exit\n'); node[0].stdin.flush()
            self.finish(node)

    def test_operation_timeout(self):
        node, connection = self.fake_coordinator()
        node[0].stdin.write(b'read 0\n')
        node[0].stdin.flush()
        self.wait_log(node, 'SEND READ_REQUEST')
        self.assertEqual(node[0].wait(timeout=7), 1, self.output(node))
        self.assertIn('Error al esperar operación', self.output(node))

    def test_peer_dies_during_request(self):
        node, connection = self.fake_coordinator()
        node[0].stdin.write(b'write 2 pending\n')
        node[0].stdin.flush()
        self.wait_log(node, 'SEND WRITE_REQUEST')
        connection.close()
        self.finish(node, expected=1)
        self.assertNotIn('RESULT WRITE', self.output(node))


if __name__ == '__main__':
    unittest.main(verbosity=2)
