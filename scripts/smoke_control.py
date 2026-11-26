"""Read a full dev-control reply, including its terminating status line."""
import socket


def command(path, line):
    with socket.socket(socket.AF_UNIX) as peer:
        peer.settimeout(60)
        peer.connect(path)
        peer.sendall((line + '\n').encode())
        with peer.makefile('rb') as stream:
            lines = []
            for raw in stream:
                if not raw.endswith(b'\n'):
                    break
                lines.append(raw)
                fields = raw.split(None, 1)
                if fields and fields[0] in (b'ok', b'err'):
                    reply = b''.join(lines).decode()
                    if fields[0] == b'err':
                        raise RuntimeError(f'{line}: {reply}')
                    return reply
    raise RuntimeError(f'Control connection closed before reply to {line}')
