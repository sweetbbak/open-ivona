"""Compile translation units with the sealed NDK r10e toolchain (bin/agcc)."""
from __future__ import annotations

import os
import shlex
import subprocess

from .project import Binary, path, ROOT


def tu_mode(b: Binary, src: str) -> str:
    unit = (b.cfg.get("units") or {}).get(src) or {}
    if "mode" in unit:
        return unit["mode"]
    modes = {f.mode for f in b.funcs if f.src == src}
    if len(modes) > 1:
        raise SystemExit(f"{src}: functions of mixed modes {modes}; set units.{src}.mode in config")
    return modes.pop() if modes else b.cfg.get("default_mode", "thumb")


def tu_flags(b: Binary, src: str) -> list[str]:
    fl = b.cfg["flags"]
    unit = (b.cfg.get("units") or {}).get(src) or {}
    mode = tu_mode(b, src)
    cxx = src.endswith((".cpp", ".cc"))
    out = list(fl["common"]) + list(fl[mode]) + list(fl["cxx" if cxx else "c"])
    out += list(fl.get(mode + "_" + ("cxx" if cxx else "c"), []))
    if "/flite/" in src:
        out.append("-Iinclude/flite")
    for r in unit.get("remove", []):
        out = [x for x in out if x != r]
    out += unit.get("add", [])
    return out


def obj_path(b: Binary, src: str) -> str:
    return path("build", b.stem, src + ".o")


def compile_cmd(b: Binary, src: str) -> list[str]:
    cc = "arm-linux-androideabi-g++" if src.endswith((".cpp", ".cc")) else "arm-linux-androideabi-gcc"
    out = os.path.relpath(obj_path(b, src), ROOT)
    return [cc, *tu_flags(b, src), *EXTRA_FLAGS, "-c", src, "-o", out]


EXTRA_FLAGS: list[str] = []  # set by `dc diff -X` for flag experiments


def compile_units(b: Binary, srcs: list[str], quiet: bool = False) -> dict[str, str | None]:
    """Compile several TUs in one container start. Returns src -> error text (None if ok)."""
    script = []
    for s in srcs:
        os.makedirs(os.path.dirname(obj_path(b, s)), exist_ok=True)
        if os.path.exists(obj_path(b, s)):
            os.remove(obj_path(b, s))
        script.append(f"echo '@@@ {s}'; " + shlex.join(compile_cmd(b, s)) + " 2>&1")
    env = dict(os.environ)
    env.setdefault("DC_NDK", b.cfg.get("toolchain", "r10e"))  # env var wins for experiments
    r = subprocess.run([path("bin", "agcc"), "sh", "-c", "; ".join(script)], capture_output=True, text=True, env=env)
    out, cur = {}, None
    for line in r.stdout.splitlines():
        if line.startswith("@@@ "):
            cur = line[4:]
            out[cur] = ""
        elif cur:
            out[cur] += line + "\n"
    res = {}
    for s in srcs:
        ok = os.path.exists(obj_path(b, s))
        msg = out.get(s, r.stderr)
        res[s] = None if ok else (msg or "compile failed")
        if msg and not quiet:
            print(msg, end="")
    return res
