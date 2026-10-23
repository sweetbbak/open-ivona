"""`dc vendor build LIB`: compile third-party code statically linked into the engine (OpenSSL,
libcurl, ...) from its pinned upstream release (external/fetch.sh) into build/libobj/<LIB>.a, so
`dc libmatch` can find and verify it like the NDK's prebuilt archives.

Recipes live in config/vendor.yaml:

  openssl:
    src: external/openssl-1.0.2h       # pristine upstream tree (never modified)
    prepare: openssl                   # tree preparation step (see PREPARE below)
    sources: openssl-libcrypto         # which files to compile (see SOURCES below)
    flags: [-marm, -O0, ...]           # added to the binary's common flags

The upstream tree is copied to build/vendor/<dir>, prepared there inside the toolchain container
(e.g. OpenSSL's Configure), and every source is compiled from its own directory so __FILE__ and
relative includes behave as in the original build.
"""
from __future__ import annotations

import glob
import os
import re
import shlex
import shutil
import subprocess

import yaml

from .project import Binary, path


def recipes() -> dict:
    with open(path("config", "vendor.yaml")) as fh:
        return yaml.safe_load(fh)


def _container(cmd: str) -> subprocess.CompletedProcess:
    return subprocess.run([path("bin", "agcc"), "sh", "-c", cmd], capture_output=True, text=True)


def _run_parallel(lib: str, cmds: list[str], jobs: int = 8) -> subprocess.CompletedProcess:
    """Run shell commands in one container start, split over `jobs` script files."""
    sdir = path("build", "vendor", lib + ".scripts")
    os.makedirs(sdir, exist_ok=True)
    names = []
    for j in range(jobs):
        fn = os.path.join(sdir, f"job{j}.sh")
        with open(fn, "w") as fh:
            fh.write("\n".join(cmds[j::jobs]) + "\n")
        names.append(os.path.relpath(fn, path()))
    return _container(" ".join(f"sh {shlex.quote(n)} &" for n in names) + " wait")


def _makefile_var(mk: str, var: str) -> list[str]:
    txt = open(mk).read().replace("\\\n", " ")
    m = re.search(rf"^{var}\s*=(.*)$", txt, re.M)
    return m.group(1).split() if m else []


# --- OpenSSL 1.0.2 ---------------------------------------------------------------------------

def _prepare_openssl(tree: str, r: dict) -> None:
    """Configure (in the container) and the header links `make links` would create."""
    res = _container(f"cd {shlex.quote(os.path.relpath(tree, path()))} && perl Configure "
                     + " ".join(r.get("configure", ["android", "no-shared"])))
    if not os.path.exists(os.path.join(tree, "crypto", "opensslconf.h")):
        raise SystemExit("OpenSSL Configure failed:\n" + res.stdout + res.stderr)
    conf = os.path.join(tree, "crypto", "opensslconf.h")
    txt = open(conf).read()
    for name in r.get("undef", []):  # settings the original opensslconf.h did not have
        txt = re.sub(rf"^#define {name}\b.*$", f"#undef {name}", txt, flags=re.M)
    open(conf, "w").write(txt)
    inc = os.path.join(tree, "include", "openssl")
    os.makedirs(inc, exist_ok=True)
    for mk in [os.path.join(tree, "crypto", "Makefile")] + glob.glob(os.path.join(tree, "crypto", "*", "Makefile")):
        for h in _makefile_var(mk, "EXHEADER"):
            src = os.path.join(os.path.dirname(mk), h)
            if "$" not in h and os.path.exists(src):
                shutil.copy(src, os.path.join(inc, h))
    shutil.copy(os.path.join(tree, "e_os2.h"), os.path.join(inc, "e_os2.h"))


def _sources_openssl_libcrypto(tree: str) -> list[tuple[str, list[str]]]:
    """(source relative to tree, per-directory include flags) for libcrypto: LIBSRC of
    crypto/Makefile and of each crypto/<dir>/Makefile listed in its SDIRS."""
    out = []
    top = os.path.join(tree, "crypto", "Makefile")

    def incs(var: str) -> list[str]:  # $(TOP) is ../.. below crypto/<dir>, .. in crypto/
        return [x.replace("$(TOP)", "..") for x in _makefile_var(top, var) if x.startswith("-I")]
    for f in _makefile_var(top, "LIBSRC"):
        out.append((f"crypto/{f}", incs("INCLUDE")))
    sub = incs("INCLUDES")  # crypto/Makefile passes INCLUDES down to every subdirectory make
    for d in _makefile_var(os.path.join(tree, "Makefile"), "SDIRS"):
        mk = os.path.join(tree, "crypto", d, "Makefile")
        if not os.path.exists(mk):
            continue
        for f in _makefile_var(mk, "LIBSRC"):
            if "$" not in f:
                out.append((f"crypto/{d}/{f}", sub))
    return out


PREPARE = {"openssl": _prepare_openssl}
SOURCES = {"openssl-libcrypto": _sources_openssl_libcrypto}


def build(stem: str, lib: str) -> None:
    r = recipes()[lib]
    b = Binary(stem)
    src = path(r["src"])
    if not os.path.isdir(src):
        raise SystemExit(f"{r['src']} missing (run external/fetch.sh)")
    tree = path("build", "vendor", os.path.basename(r["src"]))
    if os.path.exists(tree):
        shutil.rmtree(tree)
    shutil.copytree(src, tree, symlinks=True)
    if r.get("prepare"):
        PREPARE[r["prepare"]](tree, r)
    common = list(b.cfg["flags"]["common"])
    common = [x for x in common if not x.startswith("-Iinclude")]  # the engine's own headers
    common = [x for x in common if x not in r.get("remove", [])]
    flags = common + list(r.get("flags", []))
    stale = path("build", "libobj", lib)  # members libmatch saved from the previous archive
    if os.path.exists(stale):
        shutil.rmtree(stale)
    objdir = path("build", "libobj", lib + ".objs")
    if os.path.exists(objdir):
        shutil.rmtree(objdir)
    os.makedirs(objdir)
    script, objs = [], []
    for rel, incs in SOURCES[r["sources"]](tree):
        d, fn = os.path.split(rel)
        obj = os.path.join(objdir, rel.replace("/", "__")[:-2] + ".o")
        objs.append(obj)
        cwd = os.path.join(tree, d)
        cmd = ["arm-linux-androideabi-gcc", *flags, *incs, "-c", fn, "-o", os.path.relpath(obj, cwd)]
        script.append(f"(cd {shlex.quote(os.path.relpath(cwd, path()))} && {shlex.join(cmd)} "
                      f"> /dev/null 2>&1 || echo FAIL {shlex.quote(rel)})")
    res = _run_parallel(lib, script)
    fails = [l[5:] for l in res.stdout.splitlines() if l.startswith("FAIL ")]
    built = [o for o in objs if os.path.exists(o)]
    ar = path("build", "libobj", lib + ".a")
    if os.path.exists(ar):
        os.remove(ar)
    # member names stay unique: <dir>__<file>.o
    lst = path("build", "vendor", lib + ".scripts", "objs.txt")
    with open(lst, "w") as fh:
        fh.write("\n".join(os.path.relpath(o, path()) for o in built) + "\n")
    res = _container(f"arm-linux-androideabi-ar rcS {shlex.quote(os.path.relpath(ar, path()))} "
                     f"@{shlex.quote(os.path.relpath(lst, path()))}")
    if not os.path.exists(ar):
        raise SystemExit("ar failed:\n" + res.stdout + res.stderr)
    print(f"{lib}: compiled {len(built)}/{len(objs)} sources into {os.path.relpath(ar, path())}")
    if fails:
        print("failed: " + " ".join(fails))
