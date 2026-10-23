#!/usr/bin/env python3
# Copyright (C) 2026 The personal Helium fork contributors
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build and run the actual pure-C++ Live Folder policy without Chromium.

By default, sources are extracted from the repository's patch series, ensuring
the test executes the shipped implementation rather than a test reimplementation.
Use --source-root /path/to/chromium/src to test an already-applied source tree.
Only Python's standard library and a C++17 compiler are required. Nothing is
installed, downloaded, or written to the checkout while running this test.
"""

import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile


MODEL_BASE = "chrome/browser/ui/tabs/github_live_folder_model"
MODEL_FILES = (MODEL_BASE + ".h", MODEL_BASE + ".cc")


def extract_added_files(patch_path):
    """Extract only complete new model files from an ordinary unified patch."""
    lines = patch_path.read_text(encoding="utf-8").splitlines(keepends=True)
    result = {}
    index = 0
    while index < len(lines):
        if not lines[index].startswith("--- "):
            index += 1
            continue
        old_name = lines[index][4:].split("\t", 1)[0].strip()
        index += 1
        if index >= len(lines) or not lines[index].startswith("+++ "):
            continue
        new_name = lines[index][4:].split("\t", 1)[0].strip()
        if new_name.startswith("b/"):
            new_name = new_name[2:]
        index += 1
        if new_name not in MODEL_FILES:
            continue
        if old_name != "/dev/null":
            raise ValueError(f"{patch_path}: {new_name} is not a complete new file")
        if new_name in result:
            raise ValueError(f"{patch_path}: duplicate source file {new_name}")
        if index >= len(lines):
            raise ValueError(f"{patch_path}: missing addition hunk for {new_name}")
        match = re.fullmatch(r"@@ -0,0 \+1,(\d+) @@[^\n]*\n?", lines[index])
        if not match:
            raise ValueError(f"{patch_path}: unsupported addition hunk for {new_name}")
        expected_count = int(match.group(1))
        index += 1
        contents = []
        while index < len(lines) and len(contents) < expected_count:
            line = lines[index]
            if not line.startswith("+"):
                raise ValueError(f"{patch_path}: incomplete addition for {new_name}")
            contents.append(line[1:])
            index += 1
        if len(contents) != expected_count:
            raise ValueError(f"{patch_path}: truncated source file {new_name}")
        if index < len(lines) and lines[index].startswith("\\ No newline"):
            contents[-1] = contents[-1].removesuffix("\n")
            index += 1
        result[new_name] = "".join(contents)
    return result


def sources_from_patches(repo_root, patch_option, destination):
    if patch_option:
        candidates = [patch_option.resolve()]
    else:
        series = repo_root / "patches" / "series"
        candidates = []
        for line in series.read_text(encoding="utf-8").splitlines():
            line = line.split("#", 1)[0].strip()
            if line:
                candidates.append(series.parent / line)
    found = {}
    origins = []
    for patch in candidates:
        # The other patches are unrelated; avoid unnecessary extraction work.
        if MODEL_BASE not in patch.read_text(encoding="utf-8"):
            continue
        extracted = extract_added_files(patch)
        for name, contents in extracted.items():
            if name in found:
                raise ValueError(f"Multiple patches define {name}; use --source-root")
            found[name] = contents
        if extracted:
            origins.append(str(patch))
    missing = set(MODEL_FILES) - found.keys()
    if missing:
        raise ValueError(
            "Model sources absent from selected patch series: "
            + ", ".join(sorted(missing))
            + ". Use --source-root for an already-applied Chromium source tree."
        )
    for name, contents in found.items():
        output = destination / name
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(contents, encoding="utf-8")
    print("Testing shipped source from: " + ", ".join(origins), flush=True)
    return destination


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group()
    source.add_argument("--source-root", type=Path,
                        help="Chromium source root containing the model files")
    source.add_argument("--patch", type=Path,
                        help="Explicit unified patch defining the model files")
    parser.add_argument("--cxx", default=os.environ.get("CXX", "c++"),
                        help="C++ compiler command (default: CXX or c++)")
    parser.add_argument("--sanitize", action="store_true",
                        help="Enable address and undefined-behavior sanitizers")
    args = parser.parse_args()
    test_file = Path(__file__).resolve().with_name("github_live_folder_model_test.cc")
    repo_root = test_file.parent.parent
    try:
        with tempfile.TemporaryDirectory(prefix="github-live-folder-test-") as tmp:
            work = Path(tmp)
            if args.source_root:
                source_root = args.source_root.resolve()
                print(f"Testing applied source from: {source_root}", flush=True)
            else:
                source_root = sources_from_patches(repo_root, args.patch, work / "src")
            for name in MODEL_FILES:
                if not (source_root / name).is_file():
                    raise ValueError(f"Missing model source: {source_root / name}")
            compiler = shlex.split(args.cxx)
            if not compiler:
                raise ValueError("--cxx must specify a compiler")
            flags = ["-std=c++17", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-g"]
            if args.sanitize:
                flags += ["-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
            else:
                flags += ["-O2"]
            executable = work / "github_live_folder_model_test"
            command = compiler + flags + [
                "-I", str(source_root), str(source_root / MODEL_FILES[1]),
                str(test_file), "-o", str(executable),
            ]
            print("Compile: " + shlex.join(command), flush=True)
            subprocess.run(command, check=True)
            subprocess.run([str(executable)], check=True, timeout=60)
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
