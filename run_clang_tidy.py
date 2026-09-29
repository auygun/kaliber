#!/usr/bin/env python3
"""Run clang-tidy on project sources.

Usage:
  ./run_clang_tidy.py              # Run on all project sources
  ./run_clang_tidy.py --changed    # Run on files changed vs HEAD
  ./run_clang_tidy.py src/engine/engine.cc src/base/log.cc  # Run on specific files
  ./run_clang_tidy.py --fix        # Apply suggested fixes automatically
  ./run_clang_tidy.py --gen        # Regenerate compile_commands.json first
"""

import argparse
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
COMPILE_DB = ROOT / "compile_commands.json"

ALL_SOURCES = [
    "src/base/*.cc",
    "src/engine/*.cc",
    "src/engine/asset/*.cc",
    "src/engine/audio/*.cc",
    "src/engine/platform/*.cc",
    "src/engine/renderer/*.cc",
    "src/engine/renderer/vulkan/*.cc",
    "src/teapot/*.cc",
]


def generate_compile_db():
    build_dir = ROOT / "out" / "release"
    if not build_dir.exists():
        print(f"Error: {build_dir} does not exist. Run 'gn gen out/release' first.")
        sys.exit(1)
    result = subprocess.run(
        ["ninja", "-C", str(build_dir), "-t", "compdb"],
        capture_output=True, text=True
    )
    if result.returncode != 0:
        print(f"Error generating compile_commands.json:\n{result.stderr}")
        sys.exit(1)
    COMPILE_DB.write_text(result.stdout)
    print(f"Generated {COMPILE_DB}")


def get_compiled_files():
    """Return the set of source files present in compile_commands.json."""
    if not COMPILE_DB.exists():
        return set()
    entries = json.loads(COMPILE_DB.read_text())
    compiled = set()
    for entry in entries:
        p = Path(entry["file"])
        if not p.is_absolute():
            # Paths are relative to the compile directory.
            p = (Path(entry["directory"]) / p).resolve()
        try:
            compiled.add(str(p.relative_to(ROOT)))
        except ValueError:
            pass
    return compiled


def get_changed_files():
    result = subprocess.run(
        ["git", "diff", "--name-only", "--diff-filter=d", "HEAD"],
        capture_output=True, text=True, cwd=ROOT
    )
    return [
        f for f in result.stdout.strip().splitlines()
        if f.endswith(".cc") and "third_party" not in f
    ]


def expand_globs(patterns):
    files = []
    for pattern in patterns:
        files.extend(str(p.relative_to(ROOT)) for p in ROOT.glob(pattern))
    return sorted(set(files))


def main():
    parser = argparse.ArgumentParser(description="Run clang-tidy on project sources.")
    parser.add_argument("files", nargs="*", help="Specific files to check")
    parser.add_argument("--changed", action="store_true",
                        help="Only check files changed vs HEAD")
    parser.add_argument("--fix", action="store_true",
                        help="Apply suggested fixes")
    parser.add_argument("--gen", action="store_true",
                        help="Regenerate compile_commands.json before running")
    args = parser.parse_args()

    if args.gen or not COMPILE_DB.exists():
        generate_compile_db()

    if not COMPILE_DB.exists():
        print("Error: compile_commands.json not found.")
        print("Run with --gen or: ninja -C out/release -t compdb > compile_commands.json")
        sys.exit(1)

    # Only check files that are actually in the compilation database.
    # This filters out platform-specific files (e.g. platform_android.cc on Linux).
    compiled = get_compiled_files()

    if args.files:
        files = args.files
    elif args.changed:
        files = get_changed_files()
        if not files:
            print("No changed .cc files found.")
            return
    else:
        files = expand_globs(ALL_SOURCES)

    files = [f for f in files if f in compiled]
    if not files:
        print("No compilable source files to check.")
        return

    print(f"Checking {len(files)} file(s)...")

    cmd = ["clang-tidy", "-p", str(COMPILE_DB)]
    if args.fix:
        cmd.append("--fix")
    cmd.extend(files)

    sys.exit(subprocess.run(cmd, cwd=ROOT).returncode)


if __name__ == "__main__":
    main()
