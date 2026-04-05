import argparse
import re
import struct
from abc import ABC, abstractmethod
from dataclasses import dataclass, field
import serial


FRAME_HEADER  = re.compile(r'^(\d+) (\d+) (\d+)$')
SAMPLES_PER_ROW   = 16
DEFAULT_BAUD      = 1000000
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


def _frame_print(frame: Frame):
    print(f"[FRAME] tick={frame.timestamp} frame={frame.frame_number} samples={len(frame.samples)}")

    for i in range(0, len(frame.samples), SAMPLES_PER_ROW):
        row = frame.samples[i:i + SAMPLES_PER_ROW]
        print(f"  [{i:04d}]: " + " ".join(f"{s:5d}" for s in row))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--port', required=True,              help='Serial port (e.g. COM3)')
    parser.add_argument('--baud', type=int, default=DEFAULT_BAUD, help=f'Baud rate (default: {DEFAULT_BAUD})')
    args = parser.parse_args()

    reader = SerialFrameReader(debug=print)
    reader.connect(args.port, args.baud)

    print(f"Listening on {args.port} @ {args.baud} baud  (Ctrl+C to quit)")

    try:
        while True:
            frame = reader.read_frame()

            if frame:
                _frame_print(frame)
    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        reader.disconnect()


if __name__ == '__main__':
    main()
