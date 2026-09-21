#!/usr/bin/env python3
"""Compile C++ source arguments into reusable objects, then run the original link.

The wrapper accepts a normal compiler command after ``--``. It preserves the
original link argument order, but replaces each C/C++ source with a cached
object built using automatic ``-MMD -MP`` dependency files.
"""

from __future__ import annotations

import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
from typing import Sequence

SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".C"}
OBJECT_SUFFIXES = {".o", ".obj", ".a", ".so", ".dylib"}

# Options which only affect the final link and must not be passed to ``-c``.
LINK_ONLY_PREFIXES = ("-l", "-L", "-Wl,", "-fuse-ld=")
LINK_ONLY_OPTIONS = {
    "-shared",
    "-static",
    "-static-libgcc",
    "-static-libstdc++",
    "-pie",
    "-rdynamic",
    "-s",
}
LINK_ONLY_WITH_VALUE = {"-Xlinker", "-u", "-T"}


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", required=True, help="Compiler command, for example g++")
    parser.add_argument("--cache-dir", required=True, type=Path)
    parser.add_argument("--trace-file", type=Path, default=None)
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args(argv)
    if args.command and args.command[0] == "--":
        args.command = args.command[1:]
    if not args.command:
        parser.error("missing compiler arguments after --")
    return args


def compiler_command(value: str) -> list[str]:
    command = shlex.split(value)
    if not command:
        raise ValueError("compiler command is empty")
    return command


def find_output(arguments: Sequence[str]) -> Path | None:
    for index, arg in enumerate(arguments):
        if arg == "-o" and index + 1 < len(arguments):
            return Path(arguments[index + 1])
        if arg.startswith("-o") and len(arg) > 2:
            return Path(arg[2:])
    return None


def is_source(argument: str) -> bool:
    return Path(argument).suffix in SOURCE_SUFFIXES


def compile_arguments(arguments: Sequence[str]) -> list[str]:
    result: list[str] = []
    skip_next = False
    for arg in arguments:
        if skip_next:
            skip_next = False
            continue
        if is_source(arg):
            continue
        if arg == "-o":
            skip_next = True
            continue
        if arg.startswith("-o") and len(arg) > 2:
            continue
        if arg in LINK_ONLY_WITH_VALUE:
            skip_next = True
            continue
        if arg in LINK_ONLY_OPTIONS or arg.startswith(LINK_ONLY_PREFIXES):
            continue
        if Path(arg).suffix in OBJECT_SUFFIXES:
            continue
        result.append(arg)
    return result


def compiler_identity(command: Sequence[str]) -> str:
    executable = shutil.which(command[0]) or command[0]
    try:
        stat = Path(executable).resolve().stat()
        executable_state = {
            "path": str(Path(executable).resolve()),
            "size": stat.st_size,
            "mtime_ns": stat.st_mtime_ns,
        }
    except OSError:
        executable_state = {"path": executable}
    compile_environment = {
        name: os.environ.get(name, "")
        for name in (
            "CPATH",
            "CPLUS_INCLUDE_PATH",
            "C_INCLUDE_PATH",
            "OBJC_INCLUDE_PATH",
            "SDKROOT",
            "SYSROOT",
        )
    }
    return json.dumps(
        {
            "command": list(command),
            "executable": executable_state,
            "environment": compile_environment,
        },
        sort_keys=True,
    )


def object_paths(cache_dir: Path, source: Path, signature: str) -> tuple[Path, Path, Path]:
    source_key = str(source.resolve())
    digest = hashlib.sha256(f"{source_key}\0{signature}".encode()).hexdigest()
    stem = source.name.replace(os.sep, "_")
    directory = cache_dir / digest[:2] / digest[2:]
    return directory / f"{stem}.o", directory / f"{stem}.d", directory / ".lock"


def parse_depfile(depfile: Path) -> list[Path]:
    try:
        text = depfile.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError):
        return []
    text = text.replace("\\\n", " ")
    first_rule = text.splitlines()[0] if text.splitlines() else ""
    if ":" not in first_rule:
        return []
    _, dependencies = first_rule.split(":", 1)
    # Repository paths do not contain whitespace. Unescape the common Make
    # sequences so missing/newer prerequisites are still detected correctly.
    tokens = shlex.split(dependencies.replace("\\ ", " "))
    return [Path(token) for token in tokens if token]


def cache_is_fresh(object_file: Path, depfile: Path) -> bool:
    if not object_file.is_file() or not depfile.is_file():
        return False
    object_mtime = object_file.stat().st_mtime_ns
    dependencies = parse_depfile(depfile)
    if not dependencies:
        return False
    for dependency in dependencies:
        try:
            if dependency.stat().st_mtime_ns > object_mtime:
                return False
        except OSError:
            return False
    return True


def trace(path: Path | None, event: str, source: Path, object_file: Path) -> None:
    if path is None:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    record = {"event": event, "source": str(source), "object": str(object_file)}
    with path.open("a", encoding="utf-8") as handle:
        fcntl.flock(handle.fileno(), fcntl.LOCK_EX)
        handle.write(json.dumps(record, sort_keys=True) + "\n")
        handle.flush()
        fcntl.flock(handle.fileno(), fcntl.LOCK_UN)


def build_object(
    compiler: Sequence[str],
    source: Path,
    object_file: Path,
    depfile: Path,
    lock_file: Path,
    flags: Sequence[str],
    trace_file: Path | None,
) -> None:
    object_file.parent.mkdir(parents=True, exist_ok=True)
    with lock_file.open("a+") as lock:
        fcntl.flock(lock.fileno(), fcntl.LOCK_EX)
        if cache_is_fresh(object_file, depfile):
            trace(trace_file, "hit", source, object_file)
            return

        object_tmp = object_file.with_name(f".{object_file.name}.{os.getpid()}.tmp")
        dep_tmp = depfile.with_name(f".{depfile.name}.{os.getpid()}.tmp")
        command = [
            *compiler,
            *flags,
            "-MMD",
            "-MP",
            "-MF",
            str(dep_tmp),
            "-MT",
            str(object_file),
            "-c",
            str(source),
            "-o",
            str(object_tmp),
        ]
        try:
            subprocess.run(command, check=True)
            os.replace(object_tmp, object_file)
            os.replace(dep_tmp, depfile)
        finally:
            object_tmp.unlink(missing_ok=True)
            dep_tmp.unlink(missing_ok=True)
        trace(trace_file, "compile", source, object_file)


def replace_sources(arguments: Sequence[str], replacements: dict[str, Path]) -> list[str]:
    return [str(replacements[arg]) if arg in replacements else arg for arg in arguments]


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    compiler = compiler_command(args.compiler)
    original = list(args.command)
    sources = [Path(arg) for arg in original if is_source(arg)]
    output = find_output(original)

    # Preserve compiler behavior for unusual invocations which do not link a
    # source-based executable.
    if not sources or output is None or "-c" in original:
        return subprocess.run([*compiler, *original]).returncode

    flags = compile_arguments(original)
    identity = compiler_identity(compiler)
    signature = json.dumps(
        {"compiler": identity, "cwd": str(Path.cwd()), "flags": flags},
        sort_keys=True,
    )
    replacements: dict[str, Path] = {}
    for source in sources:
        object_file, depfile, lock_file = object_paths(args.cache_dir, source, signature)
        build_object(
            compiler,
            source,
            object_file,
            depfile,
            lock_file,
            flags,
            args.trace_file,
        )
        replacements[str(source)] = object_file

    output.parent.mkdir(parents=True, exist_ok=True)
    link_arguments = replace_sources(original, replacements)
    return subprocess.run([*compiler, *link_arguments]).returncode


if __name__ == "__main__":
    raise SystemExit(main())
