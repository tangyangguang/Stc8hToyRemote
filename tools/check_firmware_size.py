#!/usr/bin/env python3
import argparse
import re
import sys
from typing import NamedTuple


class MemUsage(NamedTuple):
    flash_used: int
    stack_available: int
    largest_spare_internal_ram: int


def parse_mem_usage(mem_path):
    flash_used = None
    stack_available = None
    largest_spare_internal_ram = None

    with open(mem_path, "r", encoding="utf-8", errors="ignore") as fh:
        for line in fh:
            if "ROM/EPROM/FLASH" in line:
                flash_used = int(line.split()[3])
            stack_match = re.search(r"with ([0-9]+) bytes available\.", line)
            if line.startswith("Stack starts at:") and stack_match:
                stack_available = int(stack_match.group(1))
            spare_match = re.search(
                r"The largest spare internal RAM space starts at .* with ([0-9]+) bytes available\.",
                line,
            )
            if spare_match:
                largest_spare_internal_ram = int(spare_match.group(1))
            if "No spare internal RAM space left." in line:
                largest_spare_internal_ram = 0

    if flash_used is None:
        raise ValueError("ROM/EPROM/FLASH line not found")
    if stack_available is None:
        raise ValueError("stack availability line not found")
    if largest_spare_internal_ram is None:
        raise ValueError("spare internal RAM line not found")

    return MemUsage(
        flash_used=flash_used,
        stack_available=stack_available,
        largest_spare_internal_ram=largest_spare_internal_ram,
    )


def check_limits(
    name,
    usage,
    flash_limit,
    flash_target,
    min_stack_available,
    min_largest_spare_internal_ram,
):
    errors = []

    if usage.flash_used > flash_limit:
        errors.append("%s flash %s > hard limit %s" % (name, usage.flash_used, flash_limit))
    if usage.flash_used > flash_target:
        errors.append("%s flash %s > target %s" % (name, usage.flash_used, flash_target))
    if usage.stack_available < min_stack_available:
        errors.append(
            "%s stack available %s < %s"
            % (name, usage.stack_available, min_stack_available)
        )
    if usage.largest_spare_internal_ram < min_largest_spare_internal_ram:
        errors.append(
            "%s largest spare internal RAM %s < %s"
            % (
                name,
                usage.largest_spare_internal_ram,
                min_largest_spare_internal_ram,
            )
        )

    return errors


def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument("name")
    parser.add_argument("mem_file")
    parser.add_argument("flash_limit", type=int)
    parser.add_argument("flash_target", type=int)
    parser.add_argument("min_stack_available", type=int)
    parser.add_argument("min_largest_spare_internal_ram", type=int)
    args = parser.parse_args(argv)

    usage = parse_mem_usage(args.mem_file)
    print(
        "%s flash: %s/%s target<=%s"
        % (args.name, usage.flash_used, args.flash_limit, args.flash_target)
    )
    print(
        "%s stack: %s bytes target>=%s"
        % (args.name, usage.stack_available, args.min_stack_available)
    )
    print(
        "%s largest spare internal RAM: %s bytes target>=%s"
        % (
            args.name,
            usage.largest_spare_internal_ram,
            args.min_largest_spare_internal_ram,
        )
    )

    errors = check_limits(
        name=args.name,
        usage=usage,
        flash_limit=args.flash_limit,
        flash_target=args.flash_target,
        min_stack_available=args.min_stack_available,
        min_largest_spare_internal_ram=args.min_largest_spare_internal_ram,
    )
    for error in errors:
        print(error, file=sys.stderr)
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
