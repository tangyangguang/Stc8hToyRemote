#!/usr/bin/env python3
import argparse
import subprocess
import sys
import time


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--python", required=True)
    parser.add_argument("--uploader", required=True)
    parser.add_argument("--protocol", required=True)
    parser.add_argument("--port", required=True)
    parser.add_argument("--source", required=True)
    parser.add_argument("--baud", default="19200")
    parser.add_argument("--handshake-baud", default="")
    parser.add_argument("--trim", default="")
    parser.add_argument("--attempts", default="2")
    return parser.parse_args()


def make_attempts(default_baud, default_handshake):
    attempts = []

    def add(baud, handshake):
        item = (str(baud), str(handshake))
        if item not in attempts:
            attempts.append(item)

    add(default_baud, default_handshake)
    add(default_baud, default_handshake)
    add(default_baud, "1200")
    add("9600", "")
    add("9600", "1200")
    add("4800", "1200")
    return attempts


def build_cmd(args, baud, handshake):
    cmd = [
        args.python,
        args.uploader,
        "-P",
        args.protocol,
        "-p",
        args.port,
        "-a",
        "-b",
        baud,
    ]
    if handshake:
        cmd.extend(["-l", handshake])
    if args.trim:
        cmd.extend(["-t", args.trim])
    cmd.append(args.source)
    return cmd


def main():
    args = parse_args()
    attempts = make_attempts(args.baud, args.handshake_baud)
    repeat = max(1, int(args.attempts))

    last_rc = 1
    for attempt_index, (baud, handshake) in enumerate(attempts, start=1):
        for repeat_index in range(1, repeat + 1):
            cmd = build_cmd(args, baud, handshake)
            msg = "stcgal attempt %u/%u: baud=%s" % (
                attempt_index,
                len(attempts),
                baud,
            )
            if handshake:
                msg += " handshake=%s" % handshake
            if repeat > 1:
                msg += " try=%u/%u" % (repeat_index, repeat)
            sys.stdout.write(msg + "\n")
            sys.stdout.flush()
            last_rc = subprocess.call(cmd)
            if last_rc == 0:
                return 0
            time.sleep(0.2)

    return last_rc


if __name__ == "__main__":
    raise SystemExit(main())
