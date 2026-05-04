import argparse
import re
import socket
import struct
from abc import ABC, abstractmethod
from dataclasses import dataclass, field
import serial


FRAME_HEADER  = re.compile(r'^(\d+) (\d+) (\d+)$')
SAMPLES_PER_ROW   = 16
DEFAULT_BAUD      = 2000000
DEFAULT_UDP_HOST = '0.0.0.0' # Listen on all interfaces
COMMAND_DELIMITER = '\r\n'


@dataclass
class Frame:
    timestamp:    int
    frame_number: int
    samples:      list[int] = field(default_factory=list)


class FrameReader(ABC):
    @abstractmethod
    def connect(self, *args, **kwargs):
        ...

    @abstractmethod
    def disconnect(self):
        ...

    @property
    @abstractmethod
    def connected(self) -> bool:
        ...

    @abstractmethod
    def send(self, command: str):
        ...

    @abstractmethod
    def read_frame(self) -> Frame | None:
        ...


class SerialFrameReader(FrameReader):
    def __init__(self, debug=None):
        self._port  = None
        self._debug = debug

    def connect(self, port, baud):
        self._port = serial.Serial(port, baud, timeout=1)

    def disconnect(self):
        if self._port and self._port.is_open:
            self._port.close()

        self._port = None

    @property
    def connected(self) -> bool:
        return self._port is not None and self._port.is_open

    def send(self, command: str):
        if not self.connected:
            return

        self._port.write((command + COMMAND_DELIMITER).encode('utf-8'))

    def read_frame(self) -> Frame | None:
        if not self.connected:
            return None

        line = self._port.readline().decode('utf-8', errors='replace').strip()

        if not line:
            return None

        matched = FRAME_HEADER.match(line)

        if matched:
            timestamp, frame_num, total_bytes = int(matched[1]), int(matched[2]), int(matched[3])
            raw     = self._port.read(total_bytes)
            samples = list(struct.unpack(f'<{len(raw) // 2}H', raw))

            return Frame(timestamp, frame_num, samples)

        if self._debug:
            self._debug(line)

        return None


class WirelessFrameReader(FrameReader):
    def __init__(self, debug=None):
        self._sock    = None
        self._debug   = debug
        self._pending = None
        self._cmd_addr = None
        self._last_peer = None

    def connect(self, host: str, port: int, cmd_host: str | None = None, cmd_port: int | None = None):
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._sock.bind((host, port))
        self._sock.settimeout(1.0)

        if cmd_host:
            self._cmd_addr = (cmd_host, port if cmd_port is None else cmd_port)
        else:
            self._cmd_addr = None

        self._last_peer = None

    def disconnect(self):
        if self._sock:
            self._sock.close()

        self._sock    = None
        self._pending = None
        self._cmd_addr = None
        self._last_peer = None

    @property
    def connected(self) -> bool:
        return self._sock is not None

    def send(self, command: str):
        if not self.connected:
            return

        target = self._cmd_addr or self._last_peer

        if target is None:
            if self._debug:
                self._debug(f'[UDP] command not sent (no target): {command}')
            return

        try:
            self._sock.sendto(command.encode('utf-8'), target)
        except OSError as exc:
            if self._debug:
                self._debug(f'[UDP] send failed: {exc}')

    def read_frame(self) -> Frame | None:
        if not self.connected:
            return None

        try:
            data, peer = self._sock.recvfrom(2048)
            self._last_peer = peer
        except socket.timeout:
            return None

        if self._pending is None:
            try:
                text = data.decode('utf-8').strip()
            except UnicodeDecodeError:
                if self._debug:
                    self._debug(f'[UDP] {len(data)}B binary while awaiting header – dropped')
                return None

            m = FRAME_HEADER.match(text)

            if not m:
                if self._debug:
                    self._debug(text)
                return None

            self._pending = {
                'timestamp':    int(m[1]),
                'frame_number': int(m[2]),
                'remaining':    int(m[3]),
                'buf':          bytearray(),
            }
            return None

        p = self._pending

        try:
            text = data.decode('utf-8').strip()
            if FRAME_HEADER.match(text):
                if self._debug:
                    self._debug(f'[UDP] header while frame {p["frame_number"]} incomplete – resyncing')
                m = FRAME_HEADER.match(text)
                self._pending = {
                    'timestamp':    int(m[1]),
                    'frame_number': int(m[2]),
                    'remaining':    int(m[3]),
                    'buf':          bytearray(),
                }
                return None
        except UnicodeDecodeError:
            pass

        p['buf'].extend(data)
        p['remaining'] -= len(data)

        if p['remaining'] > 0:
            return None

        raw     = bytes(p['buf'])
        n       = len(raw) // 2
        samples = list(struct.unpack(f'<{n}H', raw[:n * 2]))
        frame   = Frame(p['timestamp'], p['frame_number'], samples)
        self._pending = None
        return frame


def _frame_print(frame: Frame):
    print(f"[FRAME] tick={frame.timestamp} frame={frame.frame_number} samples={len(frame.samples)}")

    for i in range(0, len(frame.samples), SAMPLES_PER_ROW):
        row = frame.samples[i:i + SAMPLES_PER_ROW]
        print(f"  [{i:04d}]: " + " ".join(f"{s:5d}" for s in row))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--mode', choices=['uart', 'udp'], default='uart', help='Transport mode')
    parser.add_argument('--port', help='Port (UART mode "COM8", UDP mode "4210")')
    parser.add_argument('--baud', type=int, default=DEFAULT_BAUD, help=f'Baud rate (default: {DEFAULT_BAUD})')
    parser.add_argument('--host', default=DEFAULT_UDP_HOST, help=f'Listen host (UDP mode, default: {DEFAULT_UDP_HOST})')
    args = parser.parse_args()

    if args.mode == 'uart':
        reader = SerialFrameReader(debug=print)
        reader.connect(args.port, args.baud)
        print(f"Listening on {args.port} @ {args.baud} baud  (Ctrl+C to quit)")
    else:
        reader = WirelessFrameReader(debug=print)
        reader.connect(args.host, int(args.port))
        print(f"Listening on UDP {args.host}:{args.port}  (Ctrl+C to quit)")

    try:
        while True:
            frame = reader.read_frame()

            if frame:
                _frame_print(frame)
    finally:
        reader.disconnect()


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print("\nStopped.")
