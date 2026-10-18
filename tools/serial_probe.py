"""Read ESP32 UART diagnostics using macOS's standard library only."""
import argparse
import array
import fcntl
import os
import select
import termios
import time

parser = argparse.ArgumentParser()
parser.add_argument("port")
parser.add_argument("--seconds", type=float, default=15)
parser.add_argument("--reset", action="store_true", help="Reset board via RTS; does not erase flash")
parser.add_argument("--send", default="", help="Send an ASCII diagnostic command before reading")
args = parser.parse_args()
fd = os.open(args.port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
try:
    attrs = termios.tcgetattr(fd)
    attrs[0] = 0
    attrs[1] = 0
    attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    attrs[3] = 0
    attrs[4] = attrs[5] = termios.B115200
    attrs[6][termios.VMIN] = 0
    attrs[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    def modem(operation, bits):
        fcntl.ioctl(fd, operation, array.array("i", [bits]))
    modem(termios.TIOCMBIC, termios.TIOCM_DTR | termios.TIOCM_RTS)
    termios.tcflush(fd, termios.TCIFLUSH)
    if args.reset:
        modem(termios.TIOCMBIS, termios.TIOCM_RTS)
        time.sleep(0.1)
        modem(termios.TIOCMBIC, termios.TIOCM_RTS)
    if args.send:
        os.write(fd, args.send.encode("ascii"))
    until = time.monotonic() + args.seconds
    while time.monotonic() < until:
        if select.select([fd], [], [], min(0.25, max(0, until - time.monotonic())))[0]:
            print(os.read(fd, 8192).decode("utf-8", errors="replace"), end="", flush=True)
finally:
    os.close(fd)
