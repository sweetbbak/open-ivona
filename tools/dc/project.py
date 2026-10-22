"""Project paths, config and the per-binary symbol tables (TSV files under symbols/)."""
from __future__ import annotations

import csv
import hashlib
import os
import re
from dataclasses import dataclass, fields, asdict
from functools import cached_property

import yaml

from .target import TargetElf, AddrIndex

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
STATUSES = ("todo", "wip", "nonmatching", "matched", "lib")
AUTO_NAME = re.compile(r"^(?:FUN|D|sub|PTR|STR)_([0-9a-fA-F]{5,8})$")


def path(*p: str) -> str:
    return os.path.join(ROOT, *p)


@dataclass
class Func:
    addr: int
    size: int
    mode: str  # arm | thumb
    name: str
    status: str = "todo"
    src: str = ""
    score: str = ""
    note: str = ""

    @property
    def end(self) -> int:
        return self.addr + self.size


@dataclass
class Data:
    addr: int
    size: int
    name: str
    note: str = ""


def _write_tsv(fn: str, rows: list, cls) -> None:
    cols = [f.name for f in fields(cls)]
    tmp = fn + ".tmp"
    with open(tmp, "w", newline="") as fh:
        w = csv.writer(fh, delimiter="\t", lineterminator="\n")
        w.writerow(cols)
        for r in rows:
            d = asdict(r)
            w.writerow([f"{d[c]:08x}" if c in ("addr",) else (f"{d[c]:x}" if c == "size" else d[c]) for c in cols])
    os.replace(tmp, fn)


def _read_tsv(fn: str, cls) -> list:
    if not os.path.exists(fn):
        return []
    out = []
    with open(fn, newline="") as fh:
        for row in csv.DictReader(fh, delimiter="\t"):
            kw = {k: (v or "") for k, v in row.items()}
            kw["addr"] = int(kw["addr"], 16)
            kw["size"] = int(kw["size"], 16)
            out.append(cls(**kw))
    return out


class Binary:
    """One target .so plus its symbol tables and compile config."""

    def __init__(self, stem: str, so_path: str | None = None, cfg_stem: str | None = None,
                 tables_dir: str | None = None):
        """`so_path`/`cfg_stem`/`tables_dir` are overrides used by the self-test."""
        self.stem = stem
        self.so_path = so_path or path("target", stem + ".so")
        with open(path("config", (cfg_stem or stem) + ".yaml")) as fh:
            self.cfg = yaml.safe_load(fh) or {}
        tdir = tables_dir or path("symbols")
        self.funcs_path = os.path.join(tdir, stem + ".funcs.tsv")
        self.data_path = os.path.join(tdir, stem + ".data.tsv")
        self.funcs: list[Func] = _read_tsv(self.funcs_path, Func)
        self.data: list[Data] = _read_tsv(self.data_path, Data)
        if so_path is None:
            self._verify_target()

    def _verify_target(self) -> None:
        want = None
        with open(path("target", "target.sha256")) as fh:
            for line in fh:
                h, n = line.split()
                if n == self.stem + ".so":
                    want = h
        got = hashlib.sha256(open(self.so_path, "rb").read()).hexdigest()
        if want != got:
            raise SystemExit(f"target/{self.stem}.so hash mismatch (expected {want}, got {got})")

    @cached_property
    def elf(self) -> TargetElf:
        return TargetElf.load(self.so_path)

    def save(self) -> None:
        self.funcs.sort(key=lambda f: f.addr)
        self.data.sort(key=lambda d: d.addr)
        _write_tsv(self.funcs_path, self.funcs, Func)
        _write_tsv(self.data_path, self.data, Data)
        self.__dict__.pop("names", None)

    # ---- name resolution ---------------------------------------------------------
    @cached_property
    def names(self) -> "NameMap":
        return NameMap(self)

    def func(self, key: str) -> Func:
        """Look up a function by name or hex address."""
        for f in self.funcs:
            if f.name == key:
                return f
        try:
            a = int(key, 16)
        except ValueError:
            raise SystemExit(f"{self.stem}: no function named {key!r}")
        for f in self.funcs:
            if f.addr == a:
                return f
        raise SystemExit(f"{self.stem}: no function at {a:#x}")


class NameMap:
    """Bidirectional name <-> target address map: functions, data table, dynsyms, PLT imports."""

    def __init__(self, b: Binary):
        self.by_name: dict[str, int] = {}
        items: dict[int, tuple[str, int]] = {}
        for name, (val, size, typ) in b.elf.dynsyms.items():
            if typ in ("STT_FUNC", "STT_OBJECT") and val:
                v = val & ~1 if typ == "STT_FUNC" else val
                self.by_name[name] = v
                items.setdefault(v, (name, size))
        for d in b.data:
            self.by_name[d.name] = d.addr
            items[d.addr] = (d.name, d.size)
        for f in b.funcs:
            self.by_name[f.name] = f.addr
            items[f.addr] = (f.name, f.size)
        self.index = AddrIndex(items)
        self.plt = b.elf.plt
        self.elf = b.elf
        self.imports = b.elf.imports

    def addr_of(self, name: str) -> int | None:
        if name in self.by_name:
            return self.by_name[name]
        m = AUTO_NAME.match(name)
        return int(m.group(1), 16) if m else None

    def describe(self, addr: int) -> str:
        if addr in self.plt:
            return self.plt[addr] + "@plt"
        if self.elf.is_veneer(addr):
            d, _ = self.elf.follow(addr)
            return self.describe(d) + "@veneer"
        n = self.index.exact(addr)
        if n:
            return n
        c = self.index.containing(addr)
        if c:
            return f"{c[0]}+{c[1]:#x}"
        return f"D_{addr:08x}"
