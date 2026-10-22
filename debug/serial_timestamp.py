#!/usr/bin/env python3
"""Print every serial line with time elapsed since this monitor opened the port."""

import argparse
import sys
import time

import serial


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("port", help="serial port, for example COM4 or /dev/ttyACM0")
    parser.add_argument("baud", nargs="?", type=int, default=115200)
    args = parser.parse_args()

    with serial.Serial(args.port, args.baud, timeout=0.2) as port:
        started = time.monotonic()
        pending = bytearray()

        def emit(line):
            elapsed = int(time.monotonic() - started)
            hours, remainder = divmod(elapsed, 3600)
            minutes, seconds = divmod(remainder, 60)
            sys.stdout.buffer.write(
                f"[{hours:02d}:{minutes:02d}:{seconds:02d}] ".encode() + line
            )
            sys.stdout.buffer.flush()

        print(f"Monitoring {args.port} at {args.baud}; Ctrl+C to quit", file=sys.stderr)
        try:
            while True:
                pending.extend(port.read(256))
                while (newline := pending.find(b"\n")) >= 0:
                    line = bytes(pending[: newline + 1])
                    del pending[: newline + 1]
                    emit(line)
                if len(pending) > 8192:
                    emit(bytes(pending) + b"\n")
                    pending.clear()
        except KeyboardInterrupt:
            if pending:
                emit(bytes(pending) + b"\n")


if __name__ == "__main__":
    main()
