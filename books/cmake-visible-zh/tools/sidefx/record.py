#!/usr/bin/env python3
"""Record what a CMake project does, line by line, for the side-effect viewer.

    python3 -I record.py EXAMPLE_DIR -o OUT.json [--keep]

EXAMPLE_DIR holds a CMake project (CMakeLists.txt at its top, or in the run's
`src` subdirectory) and optionally `sidefx.json` describing the runs:

    {
      "title": "...",                       # shown in the viewer header
      "runs": [                             # default: one run, configure + build
        {"label": "第一次配置",
         "src": ".",                        # project directory, relative to EXAMPLE_DIR
         "build": "build",                  # build directory name (kept between runs)
         "args": ["-DUSE_FAST=OFF"],        # extra cmake arguments
         "fresh": false,                    # delete the build directory first
         "replace": {"CMakeLists.txt": "v2/CMakeLists.txt"},   # copy files before this run
         "build_targets": true,             # false: skip `cmake --build`; list: build these
         "after": [["./build/app"], ["ctest", "--test-dir", "build"]],
         "expect": "ok"}                    # "ok" | "configure-error" | "build-error"
      ],
      "step_into": ["prefix/lib/cmake"],    # extra directories (relative to the work dir)
                                            # whose .cmake files are stepped through
      "contents": ["build/build.ninja"],    # store these files even if large or internal
      "env": {"K": "V"}
    }

How it works: cmake runs under its own debugger (`--debugger`, a DAP server on a
Unix socket). The recorder stops before every command in the project's own files,
and at each stop asks for the call stack, every frame's variable scope, the cache
and the targets, and lists the files under the work directory. The difference
between two stops is the side effect of the command in between. The trace
(`--trace-expand --trace-format=json-v1`) supplies each command's expanded
arguments. After configuration, the file API (codemodel v2) gives every target's
final flags with a backtrace to the command that added them; the build is run
with `--verbose -j1` so the real commands are recorded in order.

Paths are normalised: the temporary work directory becomes /work, so the record
holds /work/src, /work/build, /work/prefix. Every CMakeLists.txt in the copy gets
one extra `return()` line at the end; stopping on it shows the effect of the
directory's last real command. Nothing else in the project is changed.
"""
from __future__ import annotations

import argparse
import fnmatch
import hashlib
import json
import os
import pty
import re
import select
import shutil
import socket
import subprocess
import sys
import tempfile
import threading
import time

WORK_TOKEN = "/work"
SENTINEL = "return()  # sidefx: end of directory\n"
# Extra directories put in front of PATH (for example where ninja lives), from the environment.
EXTRA_PATH = [d for d in os.environ.get("SIDEFX_EXTRA_PATH", "").split(os.pathsep) if d]
# A tool found outside these directories is shown at its usual place, so that
# recordings do not carry the recording machine's private paths.
STANDARD_BIN = ("/usr/bin", "/usr/local/bin", "/bin")
MAX_STEPS = 4000
CONTENT_LIMIT = 24 * 1024
# Commands whose internals are CMake's own modules: step over them.
# Everything else is stepped into; a stop outside the project's files is left with stepOut.
STEP_OVER = {"project", "enable_language", "cmake_minimum_required", "cmake_policy",
             "try_compile", "try_run"}
INTERNAL_PROPS = {"GlobalGenerator", "Makefile", "PolicyMap", "IsAIX", "IsAndroidGuiExecutable",
                  "IsAppBundleOnApple", "IsDLLPlatform", "IsExecutableWithExports",
                  "IsFrameworkOnApple", "IsImportedGloballyVisible", "IsPerConfig", "Name"}
HIDDEN_PARTS = (".cmake/api/v1/query/client-sidefx", ".cmake/api/v1/reply")


# ---------------------------------------------------------------------------
# small helpers
# ---------------------------------------------------------------------------

class Blobs:
    """Text contents stored once, referenced by a short hash."""

    def __init__(self) -> None:
        self.data: dict[str, str] = {}

    def put(self, text: str) -> str:
        key = hashlib.sha1(text.encode()).hexdigest()[:12]
        self.data.setdefault(key, text)
        return key


def is_text(raw: bytes) -> bool:
    if b"\0" in raw[:4096]:
        return False
    try:
        raw.decode("utf-8")
    except UnicodeDecodeError:
        return False
    return True


class Normalizer:
    def __init__(self, work: str, path: str = "") -> None:
        self.work = os.path.realpath(work)
        self.home = os.path.expanduser("~")
        self.tools: list[tuple[str, str]] = []
        for tool in ("ninja",):
            found = shutil.which(tool, path=path or None)
            if found and os.path.dirname(found) not in STANDARD_BIN:
                self.tools.append((found, "/usr/bin/" + tool))

    def __call__(self, value):
        if isinstance(value, str):
            v = value.replace(self.work, WORK_TOKEN)
            for real, shown in self.tools:
                v = v.replace(real, shown)
            if self.home and self.home != "/":
                v = v.replace(self.home, "~")
            return v
        if isinstance(value, list):
            return [self(v) for v in value]
        if isinstance(value, dict):
            return {self(k): self(v) for k, v in value.items()}
        return value


# ---------------------------------------------------------------------------
# DAP client
# ---------------------------------------------------------------------------

class Dap:
    def __init__(self, path: str, timeout: float = 120.0) -> None:
        self.sock = socket.socket(socket.AF_UNIX)
        self.sock.settimeout(timeout)
        self.sock.connect(path)
        self.buf = b""
        self.seq = 0
        self.events: list[dict] = []

    def send(self, command: str, arguments: dict | None = None) -> int:
        self.seq += 1
        body = json.dumps({"seq": self.seq, "type": "request", "command": command,
                           "arguments": arguments or {}}).encode()
        self.sock.sendall(b"Content-Length: %d\r\n\r\n" % len(body) + body)
        return self.seq

    def recv(self) -> dict | None:
        while True:
            if b"\r\n\r\n" in self.buf:
                head, rest = self.buf.split(b"\r\n\r\n", 1)
                n = int(head.split(b":")[1])
                if len(rest) >= n:
                    self.buf = rest[n:]
                    return json.loads(rest[:n])
            try:
                chunk = self.sock.recv(1 << 16)
            except (ConnectionResetError, OSError):
                return None
            if not chunk:
                return None
            self.buf += chunk

    def request(self, command: str, arguments: dict | None = None) -> dict | None:
        q = self.send(command, arguments)
        while True:
            m = self.recv()
            if m is None:
                return None
            if m.get("type") == "response" and m.get("request_seq") == q:
                return m
            self.events.append(m)

    def wait_event(self, names: tuple[str, ...]) -> dict | None:
        while True:
            for i, e in enumerate(self.events):
                if e.get("event") in names:
                    return self.events.pop(i)
            m = self.recv()
            if m is None:
                return None
            self.events.append(m)

    def variables(self, ref: int) -> dict[str, dict]:
        r = self.request("variables", {"variablesReference": ref})
        if not r or not r.get("success"):
            return {}
        return {v["name"]: v for v in r["body"]["variables"]}


# ---------------------------------------------------------------------------
# process output through a pty (line buffered, so messages arrive in order)
# ---------------------------------------------------------------------------

class PtyReader:
    def __init__(self, fd: int) -> None:
        self.fd = fd
        self.lock = threading.Lock()
        self.pending = b""
        self.closed = False
        self.thread = threading.Thread(target=self._run, daemon=True)
        self.thread.start()

    def _read_some(self) -> bool:
        try:
            chunk = os.read(self.fd, 1 << 16)
        except OSError:
            return False
        if not chunk:
            return False
        self.pending += chunk
        return True

    def _run(self) -> None:
        while not self.closed:
            r, _, _ = select.select([self.fd], [], [], 0.05)
            if r:
                with self.lock:
                    if not self._read_some():
                        self.closed = True

    def take(self) -> list[str]:
        """Everything written so far, as complete lines."""
        with self.lock:
            while not self.closed:
                r, _, _ = select.select([self.fd], [], [], 0)
                if not r or not self._read_some():
                    break
            text = self.pending.decode("utf-8", "replace").replace("\r\n", "\n").replace("\r", "")
            if self.closed:
                self.pending = b""
                lines = text.split("\n")
                if lines and lines[-1] == "":
                    lines.pop()
                return lines
            cut = text.rfind("\n")
            if cut < 0:
                return []
            done, rest = text[:cut], text[cut + 1:]
            self.pending = rest.encode()
            return done.split("\n")

    def finish(self) -> list[str]:
        self.thread.join(timeout=5)
        self.closed = True
        with self.lock:
            while True:
                r, _, _ = select.select([self.fd], [], [], 0)
                if not r or not self._read_some():
                    break
            text = self.pending.decode("utf-8", "replace").replace("\r\n", "\n").replace("\r", "")
            self.pending = b""
        lines = text.split("\n")
        if lines and lines[-1] == "":
            lines.pop()
        return lines


NOISE_LINES = ("Waiting for debugger client to connect", "Debugger client connected",
               "Running with debugger on")


def clean_output(lines: list[str], norm: Normalizer) -> list[str]:
    return [norm(l) for l in lines if not any(l.startswith(n) for n in NOISE_LINES)]


# ---------------------------------------------------------------------------
# state snapshots
# ---------------------------------------------------------------------------

def walk_files(work: str) -> dict[str, tuple[int, int]]:
    out = {}
    for root, dirs, files in os.walk(work):
        rel_root = os.path.relpath(root, work)
        dirs.sort()
        for name in sorted(files):
            rel = os.path.normpath(os.path.join(rel_root, name))
            if rel.startswith(".sidefx") or any(h in rel for h in HIDDEN_PARTS):
                continue
            try:
                st = os.lstat(os.path.join(root, name))
            except FileNotFoundError:
                continue
            out[rel] = (st.st_size, st.st_mtime_ns)
    return out


def flatten_target(node: dict, dap: Dap) -> dict:
    """Turn a target's variable tree into {"type":..., "props": {name: value}}."""
    children = dap.variables(node["variablesReference"]) if node.get("variablesReference") else {}
    t = {"type": node.get("value", ""), "props": {}}
    for name, v in children.items():
        if name in INTERNAL_PROPS:
            continue
        if name == "Type":
            t["type"] = v["value"]
            continue
        if name == "IsImported":
            if v["value"] == "TRUE":
                t["imported"] = True
            continue
        if name == "Properties":
            for pn, pv in dap.variables(v["variablesReference"]).items():
                t["props"][pn] = pv["value"]
            continue
        # IncludeDirectories, CompileDefinitions, Sources, LinkImplementations, ...: lists
        if v.get("variablesReference"):
            items = list(dap.variables(v["variablesReference"]).keys())
            t["props"]["@" + name] = ";".join(items)
        else:
            t["props"]["@" + name] = v.get("value", "")
    return t


def stack(dap: Dap, tid: int) -> list[dict] | None:
    st = dap.request("stackTrace", {"threadId": tid})
    if not st or not st.get("success"):
        return None
    return st["body"]["stackFrames"]


_META: dict[tuple, dict] = {}


def snapshot(dap: Dap, frames: list[dict]) -> dict:
    out = {"frames": [], "scopes": [], "cache": {}, "targets": {}}
    for i, fr in enumerate(frames):
        out["frames"].append({"file": fr.get("source", {}).get("path", ""), "line": fr["line"],
                              "name": fr["name"]})
        sc = dap.request("scopes", {"frameId": fr["id"]})
        top = dap.variables(sc["body"]["scopes"][0]["variablesReference"]) if sc else {}
        loc = dap.variables(top["Locals"]["variablesReference"]) if "Locals" in top else {}
        out["scopes"].append({k: v["value"] for k, v in loc.items()})
        if i == 0:
            if "CacheVariables" in top:
                for key, v in dap.variables(top["CacheVariables"]["variablesReference"]).items():
                    name, _, typ = key.rpartition(":")
                    mk = (key, v["value"])
                    if mk not in _META:
                        _META[mk] = dap.variables(v["variablesReference"]) if v.get("variablesReference") else {}
                    meta = _META[mk]
                    out["cache"][name or key] = {
                        "type": typ, "value": v["value"],
                        "help": meta.get("HELPSTRING", {}).get("value", ""),
                        "advanced": meta.get("ADVANCED", {}).get("value", "") in ("1", "ON", "TRUE"),
                    }
            if "Targets" in top:
                for name, v in dap.variables(top["Targets"]["variablesReference"]).items():
                    out["targets"][name] = flatten_target(v, dap)
    # frames come innermost first; store outermost first
    out["frames"].reverse()
    out["scopes"].reverse()
    return out


# ---------------------------------------------------------------------------
# diffs
# ---------------------------------------------------------------------------

def dict_diff(old: dict, new: dict) -> list[dict]:
    ch = []
    for k in sorted(set(old) | set(new)):
        if k not in new:
            ch.append({"name": k, "op": "del", "old": old[k]})
        elif k not in old:
            ch.append({"name": k, "op": "add", "new": new[k]})
        elif old[k] != new[k]:
            ch.append({"name": k, "op": "set", "old": old[k], "new": new[k]})
    return ch


def frame_key(f: dict) -> tuple:
    return (f["file"], f["line"], f["name"])


def common_depth(a: list[dict], b: list[dict]) -> int:
    """Number of outer frames that are the same frame in both stacks.

    Frame 0 is the top-level CMakeLists.txt. Frame i (i >= 1) is the same frame when
    the frame below it is at the same call site and frame i runs the same file;
    the innermost frame's line moves as execution goes on, so it is not compared.
    """
    if not a or not b or a[0]["file"] != b[0]["file"]:
        return 0
    n = 1
    for i in range(1, min(len(a), len(b))):
        if frame_key(a[i - 1]) != frame_key(b[i - 1]) or a[i]["file"] != b[i]["file"]:
            break
        n += 1
    return n


# ---------------------------------------------------------------------------
# file API
# ---------------------------------------------------------------------------

def write_query(build: str) -> None:
    q = os.path.join(build, ".cmake", "api", "v1", "query", "client-sidefx")
    os.makedirs(q, exist_ok=True)
    with open(os.path.join(q, "query.json"), "w") as f:
        json.dump({"requests": [{"kind": "codemodel", "version": 2},
                                {"kind": "cache", "version": 2},
                                {"kind": "cmakeFiles", "version": 1}]}, f)


def read_reply(build: str, norm: Normalizer) -> dict | None:
    rdir = os.path.join(build, ".cmake", "api", "v1", "reply")
    if not os.path.isdir(rdir):
        return None
    idx = sorted(f for f in os.listdir(rdir) if f.startswith("index-"))
    if not idx:
        return None
    with open(os.path.join(rdir, idx[-1])) as f:
        index = json.load(f)
    resp = index.get("reply", {}).get("client-sidefx", {}).get("query.json", {}).get("responses", [])
    out: dict = {"targets": [], "directories": []}
    for r in resp:
        if r.get("kind") != "codemodel" or "jsonFile" not in r:
            continue
        with open(os.path.join(rdir, r["jsonFile"])) as f:
            cm = json.load(f)
        cfg = cm["configurations"][0]
        out["config"] = cfg.get("name", "")
        for t in cfg.get("targets", []):
            with open(os.path.join(rdir, t["jsonFile"])) as f:
                out["targets"].append(simplify_target(json.load(f)))
        for d in cfg.get("directories", []):
            if "jsonFile" in d:
                with open(os.path.join(rdir, d["jsonFile"])) as f:
                    dj = json.load(f)
                out["directories"].append(simplify_directory(d, dj))
    return norm(out)


def backtraces(graph: dict):
    nodes, files, cmds = graph.get("nodes", []), graph.get("files", []), graph.get("commands", [])

    def bt(i):
        chain = []
        while i is not None and 0 <= i < len(nodes):
            n = nodes[i]
            item = {"file": files[n["file"]], "line": n.get("line", 0)}
            if "command" in n:
                item["cmd"] = cmds[n["command"]]
            chain.append(item)
            i = n.get("parent")
        return chain
    return bt


def simplify_target(t: dict) -> dict:
    bt = backtraces(t.get("backtraceGraph", {}))
    s = {"name": t["name"], "type": t["type"], "id": t.get("id", ""),
         "bt": bt(t.get("backtrace")),
         "artifacts": [a["path"] for a in t.get("artifacts", [])],
         "nameOnDisk": t.get("nameOnDisk", ""),
         "sources": [{"path": x["path"], "group": x.get("compileGroupIndex"),
                      "generated": x.get("isGenerated", False), "bt": bt(x.get("backtrace"))}
                     for x in t.get("sources", [])],
         "deps": [{"id": d["id"], "bt": bt(d.get("backtrace"))} for d in t.get("dependencies", [])],
         "groups": []}
    for g in t.get("compileGroups", []):
        s["groups"].append({
            "lang": g.get("language", ""),
            "flags": [{"v": f["fragment"], "bt": bt(f.get("backtrace"))}
                      for f in g.get("compileCommandFragments", [])],
            "includes": [{"v": i["path"], "system": i.get("isSystem", False), "bt": bt(i.get("backtrace"))}
                         for i in g.get("includes", [])],
            "defines": [{"v": d["define"], "bt": bt(d.get("backtrace"))} for d in g.get("defines", [])],
            "std": (g.get("languageStandard") or {}).get("standard", ""),
            "stdBt": [bt(b) for b in (g.get("languageStandard") or {}).get("backtraces", [])],
            "sources": g.get("sourceIndexes", []),
        })
    for key in ("link", "archive"):
        if key in t:
            s[key] = [{"v": f["fragment"], "role": f.get("role", ""), "bt": bt(f.get("backtrace"))}
                      for f in t[key].get("commandFragments", [])]
    if "install" in t:
        s["install"] = {"prefix": t["install"].get("prefix", {}).get("path", ""),
                        "dest": [{"path": d["path"], "bt": bt(d.get("backtrace"))}
                                 for d in t["install"].get("destinations", [])]}
    for key in ("linkLibraries", "interfaceLinkLibraries", "compileDependencies",
                "interfaceCompileDependencies", "objectDependencies", "orderDependencies"):
        if key in t:
            s[key] = [{"id": x.get("id", ""), "fragment": x.get("fragment", ""),
                       "bt": bt(x.get("backtrace"))} for x in t[key]]
    return s


def simplify_directory(d: dict, dj: dict) -> dict:
    bt = backtraces(dj.get("backtraceGraph", {}))
    inst = []
    for i in dj.get("installers", []):
        inst.append({"component": i.get("component", ""), "type": i.get("type", ""),
                     "dest": i.get("destination", ""), "paths": i.get("paths", []),
                     "target": i.get("targetId", ""), "export": i.get("exportName", ""),
                     "bt": bt(i.get("backtrace"))})
    return {"source": d.get("source", ""), "build": d.get("build", ""), "installers": inst}


# ---------------------------------------------------------------------------
# trace
# ---------------------------------------------------------------------------

def read_trace(path: str) -> list[dict]:
    out = []
    if not os.path.exists(path):
        return out
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            try:
                d = json.loads(line)
            except json.JSONDecodeError:
                continue
            if "cmd" in d:
                out.append(d)
    return out


# ---------------------------------------------------------------------------
# imported and ALIAS targets, from the trace
# ---------------------------------------------------------------------------

LIB_TYPES = {"STATIC": "STATIC_LIBRARY", "SHARED": "SHARED_LIBRARY", "MODULE": "MODULE_LIBRARY",
             "UNKNOWN": "UNKNOWN_LIBRARY", "INTERFACE": "INTERFACE_LIBRARY", "OBJECT": "OBJECT_LIBRARY"}
USAGE_CMDS = {"target_include_directories": "INTERFACE_INCLUDE_DIRECTORIES",
              "target_compile_definitions": "INTERFACE_COMPILE_DEFINITIONS",
              "target_compile_options": "INTERFACE_COMPILE_OPTIONS",
              "target_compile_features": "INTERFACE_COMPILE_FEATURES",
              "target_link_libraries": "INTERFACE_LINK_LIBRARIES",
              "target_link_options": "INTERFACE_LINK_OPTIONS",
              "target_link_directories": "INTERFACE_LINK_DIRECTORIES",
              "target_sources": "INTERFACE_SOURCES"}


def _append(props: dict, key: str, values: list[str], mode: str = "APPEND") -> None:
    new = ";".join(values)
    old = props.get(key, "")
    if mode == "SET" or not old:
        props[key] = new
    elif mode == "APPEND_STRING":
        props[key] = old + new
    else:
        props[key] = old + ";" + new if new else old


def apply_imported(imp: dict, t: dict, dname: str) -> bool:
    """Update `imp` with one trace record. Returns True if anything changed."""
    cmd, args = t.get("cmd", "").lower(), t.get("args", [])
    if not args:
        return False
    if cmd in ("add_library", "add_executable") and "IMPORTED" in args[1:]:
        typ = "EXECUTABLE" if cmd == "add_executable" else LIB_TYPES.get(args[1] if len(args) > 1 else "", "UNKNOWN_LIBRARY")
        imp[args[0]] = {"type": typ, "props": {}, "imported": True, "dir": dname, "fromTrace": True}
        if "GLOBAL" in args:
            imp[args[0]]["props"]["IMPORTED_GLOBAL"] = "TRUE"
        return True
    if cmd == "add_library" and len(args) >= 3 and args[1] == "ALIAS":
        imp[args[0]] = {"type": "ALIAS", "props": {"ALIASED_TARGET": args[2]}, "alias": True, "dir": dname,
                        "fromTrace": True}
        return True
    if cmd == "set_target_properties" and "PROPERTIES" in args:
        i = args.index("PROPERTIES")
        names, kv = args[:i], args[i + 1:]
        hit = False
        for n in names:
            if n in imp and not imp[n].get("alias"):
                for j in range(0, len(kv) - 1, 2):
                    imp[n]["props"][kv[j]] = kv[j + 1]
                hit = True
        return hit
    if cmd == "set_property" and args[0] == "TARGET" and "PROPERTY" in args:
        i = args.index("PROPERTY")
        head, rest = args[1:i], args[i + 1:]
        mode = "SET"
        for m in ("APPEND", "APPEND_STRING"):
            if m in head:
                mode = m
                head = [h for h in head if h != m]
        if not rest:
            return False
        hit = False
        for n in head:
            if n in imp and not imp[n].get("alias"):
                _append(imp[n]["props"], rest[0], rest[1:], mode)
                hit = True
        return hit
    if cmd in USAGE_CMDS and args[0] in imp and not imp[args[0]].get("alias"):
        prop = USAGE_CMDS[cmd]
        vals, on = [], False
        for a in args[1:]:
            if a in ("INTERFACE", "PUBLIC", "PRIVATE"):
                on = a == "INTERFACE"
                continue
            if a in ("SYSTEM", "BEFORE", "AFTER"):
                if a == "SYSTEM" and cmd == "target_include_directories":
                    prop = "INTERFACE_SYSTEM_INCLUDE_DIRECTORIES"
                continue
            if on:
                vals.append(a)
        if vals:
            _append(imp[args[0]]["props"], prop, vals)
            return True
    return False


# ---------------------------------------------------------------------------
# one run
# ---------------------------------------------------------------------------

class Recorder:
    def __init__(self, example: str, manifest: dict, work: str, keep: bool) -> None:
        self.example = os.path.realpath(example)
        self.manifest = manifest
        self.work = os.path.realpath(work)
        self.keep = keep
        self.blobs = Blobs()
        self.env = dict(os.environ)
        self.env.update({"CC": "gcc", "CXX": "g++", "LC_ALL": "C.UTF-8", "LANG": "C.UTF-8",
                         "TERM": "dumb"})
        for k in ("CFLAGS", "CXXFLAGS", "LDFLAGS", "CMAKE_GENERATOR", "CMAKE_BUILD_TYPE",
                  "CMAKE_PREFIX_PATH", "CMAKE_EXPORT_COMPILE_COMMANDS", "CMAKE_COLOR_DIAGNOSTICS",
                  "CLICOLOR_FORCE", "CMAKE_INSTALL_PREFIX", "CMAKE_TOOLCHAIN_FILE"):
            self.env.pop(k, None)
        self.env["PATH"] = os.pathsep.join(EXTRA_PATH + [self.env.get("PATH", "")])
        self.norm = Normalizer(self.work, self.env["PATH"])
        self.env.update({k: re.sub(r"(?<![\w/])/work(?=/|$)", self.work, v) for k, v in manifest.get("env", {}).items()})
        self.contents_globs = manifest.get("contents", [])
        self.file_versions: dict[str, tuple[int, int]] = {}
        self.sentinels: dict[str, int] = {}

    # -- files -------------------------------------------------------------

    def real(self, v):
        """Arguments in sidefx.json may name /work/...; map them to the real work directory."""
        if isinstance(v, list):
            return [self.real(x) for x in v]
        if isinstance(v, str):
            return re.sub(r"(?<![\w/])/work(?=/|$)", self.work, v)
        return v

    def keep_content(self, rel: str, size: int) -> bool:
        if any(fnmatch.fnmatch(rel, g) for g in self.contents_globs):
            return True
        if size > CONTENT_LIMIT:
            return False
        parts = rel.split(os.sep)
        if "CMakeFiles" in parts or ".cmake" in parts:
            return False
        base = parts[-1]
        if base in ("build.ninja", "Makefile", ".ninja_log", ".ninja_deps", "CTestTestfile.cmake"):
            return False
        return True

    def file_entry(self, rel: str, size: int) -> dict:
        e = {"path": rel, "size": size}
        full = os.path.join(self.work, rel)
        if self.keep_content(rel, size) and os.path.isfile(full):
            try:
                with open(full, "rb") as f:
                    raw = f.read()
            except OSError:
                return e
            if is_text(raw):
                text = raw.decode("utf-8")
                if rel in self.sentinels:
                    text = text[: len(text) - len(SENTINEL)]
                    e["size"] = len(text.encode())
                e["blob"] = self.blobs.put(self.norm(text))
        return e

    def file_diff(self, old: dict, new: dict) -> list[dict]:
        out = []
        for rel in sorted(set(old) | set(new)):
            if rel not in new:
                out.append({"path": rel, "op": "del"})
            elif rel not in old:
                out.append(dict(self.file_entry(rel, new[rel][0]), op="add"))
            elif old[rel] != new[rel]:
                out.append(dict(self.file_entry(rel, new[rel][0]), op="mod"))
        return out

    def source_listing(self, files: dict) -> list[dict]:
        return [self.file_entry(rel, size) for rel, (size, _) in sorted(files.items())]

    # -- prepare -----------------------------------------------------------

    def prepare(self) -> None:
        dst = os.path.join(self.work, "src")
        shutil.copytree(self.example, dst, symlinks=False,
                        ignore=shutil.ignore_patterns("sidefx.json", "*.sidefx.json", ".sidefx*",
                                                      "build", "build-*", "__pycache__"))
        os.makedirs(os.path.join(self.work, ".sidefx"), exist_ok=True)
        self.add_sentinels()

    def add_sentinels(self) -> None:
        src = os.path.join(self.work, "src")
        for root, _, files in os.walk(src):
            if "CMakeLists.txt" not in files:
                continue
            path = os.path.join(root, "CMakeLists.txt")
            rel = os.path.relpath(path, self.work)
            with open(path, encoding="utf-8") as f:
                text = f.read()
            if rel in self.sentinels:          # re-added after a `replace`
                if text.endswith(SENTINEL):
                    continue
            if text and not text.endswith("\n"):
                text += "\n"
            self.sentinels[rel] = text.count("\n") + 1
            with open(path, "w", encoding="utf-8") as f:
                f.write(text + SENTINEL)

    def apply_replace(self, replace: dict) -> list[str]:
        changed = []
        for target, source in replace.items():
            dst = os.path.join(self.work, "src", target)
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            shutil.copyfile(os.path.join(self.example, source), dst)
            changed.append(os.path.join("src", target))
            self.sentinels.pop(os.path.join("src", target), None)
        self.add_sentinels()
        return changed

    # -- stepping ------------------------------------------------------------

    def in_step_files(self, path: str, run_src: str) -> bool:
        if not path:
            return False
        p = os.path.realpath(path)
        roots = [run_src] + [os.path.join(self.work, d) for d in self.manifest.get("step_into", [])]
        return any(p == r or p.startswith(r + os.sep) for r in roots)

    def configure(self, run: dict, index: int) -> dict:
        src_rel = os.path.normpath(os.path.join("src", run.get("src", ".")))
        build_rel = run.get("build", "build")
        run_src = os.path.join(self.work, src_rel)
        build = os.path.join(self.work, build_rel)
        if run.get("fresh") and os.path.isdir(build):
            shutil.rmtree(build)
        write_query(build)
        sock = os.path.join(tempfile.gettempdir(), "sidefx-%d-%d.sock" % (os.getpid(), index))
        trace = os.path.join(self.work, ".sidefx", "trace-%d.json" % index)
        if os.path.exists(sock):
            os.unlink(sock)
        shown = ["cmake", "-S", src_rel, "-B", build_rel]
        if not os.path.exists(os.path.join(build, "CMakeCache.txt")) and "-G" not in run.get("args", []):
            shown += ["-G", "Ninja"]
        shown += list(run.get("args", []))
        argv = shown[:1] + ["--debugger", "--debugger-pipe", sock, "--trace-expand",
                            "--trace-format=json-v1", "--trace-redirect=" + trace] + self.real(shown[1:])
        master, slave = pty.openpty()
        proc = subprocess.Popen(argv, cwd=self.work, env=self.env, stdin=subprocess.DEVNULL,
                                stdout=slave, stderr=slave, close_fds=True)
        os.close(slave)
        reader = PtyReader(master)
        for _ in range(400):
            if os.path.exists(sock) or proc.poll() is not None:
                break
            time.sleep(0.01)
        rec = {"argv": shown, "steps": [], "frames0": []}
        files_before = walk_files(self.work)
        if proc.poll() is not None:
            rec["exit"] = proc.wait()
            rec["output"] = clean_output(reader.finish(), self.norm)
            return rec
        dap = Dap(sock)
        dap.request("initialize", {"clientID": "sidefx", "adapterID": "cmake", "linesStartAt1": True})
        top = os.path.join(run_src, "CMakeLists.txt")
        nlines = self.sentinels.get(os.path.relpath(top, self.work), 400)
        dap.request("setBreakpoints", {"source": {"path": top},
                                       "breakpoints": [{"line": i} for i in range(1, nlines + 1)]})
        dap.request("setExceptionBreakpoints", {"filters": []})
        dap.request("configurationDone")
        stops: list[dict] = []
        ev = dap.wait_event(("stopped", "exited", "terminated"))
        tid = (ev or {}).get("body", {}).get("threadId", 1)
        cleared = False
        pending_out: list[str] = []
        while ev and ev.get("event") == "stopped" and len(stops) < MAX_STEPS:
            if not cleared:
                dap.request("setBreakpoints", {"source": {"path": top}, "breakpoints": []})
                cleared = True
            frames = stack(dap, tid)
            if not frames:
                break
            topf = frames[0]
            if self.in_step_files(topf.get("source", {}).get("path", ""), run_src):
                snap = snapshot(dap, frames)
                pending_out += reader.take()
                snap["files"] = walk_files(self.work)
                snap["out"] = pending_out
                pending_out = []
                stops.append(snap)
                action = "next" if topf["name"] in STEP_OVER else "stepIn"
            else:
                action = "stepOut"
            dap.request(action, {"threadId": tid})
            ev = dap.wait_event(("stopped", "exited", "terminated"))
        if ev and ev.get("event") == "stopped":       # step limit reached
            dap.request("continue", {"threadId": tid})
        try:
            dap.request("disconnect", {})
        except OSError:
            pass
        code = proc.wait(timeout=600)
        tail = pending_out + reader.finish()
        files_after = walk_files(self.work)
        if os.path.exists(sock):
            os.unlink(sock)
        rec["exit"] = code
        records = read_trace(trace)
        self.scan_definitions(records)
        rec["inner_total"] = len(records)
        rec["steps"], rec["state0"] = self.build_steps(stops, files_before, records, run_src)
        # Output after the last stop: messages from the last command, then the generate phase.
        gen_at = next((i for i, l in enumerate(tail) if l.startswith("-- Configuring done")
                       or l.startswith("-- Configuring incomplete")), None)
        if rec["steps"] and gen_at is not None:
            rec["steps"][-1]["fx"].setdefault("out", []).extend(clean_output(tail[:gen_at], self.norm))
            tail = tail[gen_at:]
        rec["tail"] = clean_output(tail, self.norm)
        last_files = stops[-1]["files"] if stops else files_before
        rec["generate"] = {"files": self.file_diff(last_files, files_after)}
        rec["codemodel"] = read_reply(build, self.norm)
        rec["files_end"] = files_after
        return rec

    def build_steps(self, stops: list[dict], files_before: dict, trace: list[dict],
                    run_src: str) -> tuple[list[dict], dict]:
        """Turn the stops into steps with the effects of each command."""
        if not stops:
            return [], {}
        # Align trace records (user files only) with the stops.
        user = [(i, t) for i, t in enumerate(trace) if self.in_step_files(t.get("file", ""), run_src)]
        # prefix count of commands outside the project's files (CMake's own modules)
        foreign = [0]
        user_idx = {i for i, _ in user}
        for i in range(len(trace)):
            foreign.append(foreign[-1] + (0 if i in user_idx else 1))
        steps = []
        ti = 0
        prev_trace_idx = -1
        for k, s in enumerate(stops):
            topf = s["frames"][-1]
            rel = os.path.relpath(os.path.realpath(topf["file"]), self.work)
            step = {"file": rel, "line": topf["line"], "cmd": topf["name"], "depth": len(s["frames"]),
                    "stack": [{"file": os.path.relpath(os.path.realpath(f["file"]), self.work)
                               if f["file"] else "", "line": f["line"], "name": f["name"]}
                              for f in s["frames"][:-1]]}
            if self.sentinels.get(rel) == topf["line"]:
                step["sentinel"] = True
            j = ti
            while j < len(user):
                t = user[j][1]
                if os.path.realpath(t["file"]) == os.path.realpath(topf["file"]) and t["line"] == topf["line"]:
                    break
                j += 1
            if j < len(user):
                t = user[j][1]
                step["args"] = self.norm(t.get("args", []))
                if t.get("line_end") and t["line_end"] != t["line"]:
                    step["line_end"] = t["line_end"]
                idx = user[j][0]
                step["_tidx"] = idx
                if k > 0:
                    n_inner = foreign[idx] - foreign[prev_trace_idx + 1]
                    if n_inner:
                        steps[-1]["inner"] = n_inner
                prev_trace_idx = idx
                ti = j + 1
            steps.append(step)
        # The debugger lists only the targets of the directory being processed.
        # Targets are global and never deleted, so keep every target seen so far.
        table: dict = {}
        for s in stops:
            dname = self.scope_label(s["frames"], len(s["frames"]) - 1)
            for name, t in s["targets"].items():
                if name not in table:
                    t = dict(t, dir=self.rel(os.path.dirname(s["frames"][-1]["file"]))
                             if s["frames"] else "")
                else:
                    t = dict(t, dir=table[name].get("dir", ""))
                table[name] = t
            s["targets"] = dict(table)
        # Imported and ALIAS targets are not listed by the debugger at all. Rebuild them
        # from the trace: the commands that create them and set their properties, wherever
        # they ran (Find modules, package config files, the project itself).
        imp: dict = {}
        for k, s in enumerate(stops):
            s["targets"] = dict(imp, **s["targets"])
            lo = steps[k].get("_tidx")
            if lo is None:
                continue
            hi = next((steps[j]["_tidx"] for j in range(k + 1, len(steps)) if "_tidx" in steps[j]), len(trace))
            dname = self.rel(os.path.dirname(s["frames"][-1]["file"])) if s["frames"] else ""
            changed = False
            for t in trace[lo:hi]:
                changed |= apply_imported(imp, t, dname)
            if changed:
                imp = {n: dict(v, props=dict(v["props"])) for n, v in imp.items()}
        for st in steps:
            st.pop("_tidx", None)
        # Effects: difference between this stop and the next.
        for k, s in enumerate(stops):
            nxt = stops[k + 1] if k + 1 < len(stops) else None
            fx: dict = {}
            if nxt is not None:
                fx.update(self.scope_effects(s, nxt))
                cache = dict_diff(s["cache"], nxt["cache"])
                if cache:
                    fx["cache"] = cache
                tg = self.target_effects(s["targets"], nxt["targets"])
                if tg:
                    fx["targets"] = tg
                files = self.file_diff(s["files"], nxt["files"])
                if files:
                    fx["files"] = files
                out = [l for l in clean_output(nxt.get("out", []), self.norm)]
                if any(out):
                    fx["out"] = out
            steps[k]["fx"] = self.norm(fx)
            if k > 0:
                d = self.norm(dict_diff(stops[k - 1]["scopes"][-1], s["scopes"][-1]))
                pfx = steps[k - 1]["fx"]
                implied = []
                if "entered" not in pfx and "left" not in pfx:
                    implied = next((v["changes"] for v in pfx.get("vars", []) if v.get("top")), [])
                if d != implied:
                    steps[k]["scope"] = d
            steps[k]["scopeLabel"] = self.scope_label(s["frames"], len(s["frames"]) - 1)
        first = stops[0]
        state0 = self.norm({
            "scopes": [first["scopes"][-1]],
            "cache": first["cache"],
            "targets": first["targets"],
            "files": self.source_listing(first["files"]),
        })
        pre = self.file_diff(files_before, first["files"])
        if pre:
            state0["preFiles"] = pre
        return steps, state0

    def scope_label(self, frames: list[dict], level: int) -> dict:
        """Describe the variable scope that frame `level` runs in."""
        kind, name = "dir", os.path.relpath(os.path.dirname(os.path.realpath(frames[0]["file"])), self.work)
        for i in range(1, level + 1):
            caller = frames[i - 1]["name"]
            fdir = os.path.relpath(os.path.dirname(os.path.realpath(frames[i]["file"])), self.work)
            if caller == "add_subdirectory":
                kind, name = "dir", fdir
            elif caller in ("include", "find_package", "macro") or caller in self.macros:
                continue
            elif caller in self.functions:
                kind, name = "function", caller
        return {"kind": kind, "name": name}

    def scope_effects(self, a: dict, b: dict) -> dict:
        """Variable changes between two stops, per scope, plus scopes entered and left."""
        fa, fb = a["frames"], b["frames"]
        n = common_depth(fa, fb)
        fx: dict = {}
        vars_ch = []
        top_chain = n == len(fa) == len(fb)        # still the innermost scope
        for lvl in range(n - 1, -1, -1):
            sa, sb = a["scopes"][lvl], b["scopes"][lvl]
            # include/macro frames share their caller's scope: report it once, at the caller
            if lvl > 0 and a["scopes"][lvl - 1] == sa and b["scopes"][lvl - 1] == sb:
                continue
            ch = dict_diff(sa, sb)
            if ch:
                e = {"scope": self.scope_label(fa, lvl), "changes": ch}
                if top_chain:
                    e["top"] = True
                vars_ch.append(e)
            top_chain = False
        if vars_ch:
            fx["vars"] = vars_ch
        left, dropped = [], []
        survivor = b["scopes"][n - 1] if n > 0 else {}
        for lvl in range(len(fa) - 1, n - 1, -1):
            label = self.scope_label(fa, lvl)
            left.append({"file": self.rel(fa[lvl]["file"]), "scope": label})
            if lvl > 0 and label != self.scope_label(fa, lvl - 1):
                own = a["scopes"][lvl]
                dropped.append({"scope": label,
                                "names": sorted(k for k in own if k not in survivor),
                                "shadowed": sorted(k for k in own if k in survivor and own[k] != survivor[k])})
        entered = []
        for lvl in range(n, len(fb)):
            parent = b["scopes"][lvl - 1] if lvl > 0 else {}
            entered.append({"file": self.rel(fb[lvl]["file"]), "via": fb[lvl - 1]["name"] if lvl > 0 else "",
                            "scope": self.scope_label(fb, lvl),
                            "changes": dict_diff(parent, b["scopes"][lvl])})
        if left:
            fx["left"] = left
        if entered:
            fx["entered"] = entered
        if dropped:
            fx["dropped"] = dropped
        return fx

    def rel(self, path: str) -> str:
        return os.path.relpath(os.path.realpath(path), self.work) if path else ""

    def target_effects(self, old: dict, new: dict) -> list[dict]:
        out = []
        for name in sorted(set(old) | set(new)):
            if name not in old:
                t = new[name]
                e = {"name": name, "op": "add", "type": t["type"], "props": t["props"], "dir": t.get("dir", "")}
                for flag in ("imported", "alias", "fromTrace"):
                    if t.get(flag):
                        e[flag] = True
                out.append(e)
            elif name not in new:
                out.append({"name": name, "op": "del"})
            else:
                ch = dict_diff(old[name]["props"], new[name]["props"])
                if ch:
                    out.append({"name": name, "op": "set", "changes": ch})
        return out

    # -- definitions of functions/macros (to tell scopes apart) ------------

    def scan_definitions(self, trace: list[dict]) -> None:
        for t in trace:
            if t.get("cmd") == "function" and t.get("args"):
                self.functions.add(t["args"][0])
            elif t.get("cmd") == "macro" and t.get("args"):
                self.macros.add(t["args"][0])

    # -- build and after-commands ------------------------------------------

    def run_cmd(self, argv: list[str], label: str, timeout: int = 600) -> dict:
        before = walk_files(self.work)
        t0 = time.time()
        p = subprocess.run(self.real(argv), cwd=self.work, env=self.env, stdin=subprocess.DEVNULL,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout)
        after = walk_files(self.work)
        text = p.stdout.decode("utf-8", "replace")
        return {"label": label, "argv": argv, "exit": p.returncode,
                "output": clean_output(text.splitlines(), self.norm),
                "files": self.file_diff(before, after), "seconds": round(time.time() - t0, 2)}

    def build(self, run: dict) -> dict:
        build_rel = run.get("build", "build")
        argv = ["cmake", "--build", build_rel, "--verbose", "-j", "1"]
        targets = run.get("build_targets", True)
        if isinstance(targets, list):
            for t in targets:
                argv += ["--target", t]
        res = self.run_cmd(argv, "构建")
        res["commands"] = parse_ninja(res["output"])
        return res

    # -- whole record --------------------------------------------------------

    def record(self) -> dict:
        self.functions: set[str] = set()
        self.macros: set[str] = set()
        self.prepare()
        runs_spec = self.manifest.get("runs") or [{}]
        sources = None
        record = {"version": 1, "title": self.manifest.get("title", ""),
                  "cmake": cmake_version(), "generator": "Ninja",
                  "compiler": compiler_version(self.env), "runs": []}
        for i, run in enumerate(runs_spec):
            changed = self.apply_replace(run["replace"]) if run.get("replace") else []
            files_now = walk_files(self.work)
            src_files = {k: v for k, v in files_now.items() if k.startswith("src" + os.sep)}
            listing = self.source_listing(src_files)
            # Pre-scan definitions from a dry trace is not possible; use the previous runs
            # plus this run's trace after the fact (scopes are relabelled below).
            rec = {"label": run.get("label", "配置并构建" if i == 0 else "第 %d 次运行" % (i + 1)),
                   "src": os.path.normpath(os.path.join("src", run.get("src", "."))),
                   "build": run.get("build", "build")}
            if sources is None or changed:
                rec["sources"] = listing
                sources = listing
            if changed:
                rec["replaced"] = changed
            if run.get("configure", True) is False:
                # only `cmake --build`: shows the build re-running cmake by itself
                cfg = {"skipped": True, "argv": [], "steps": [], "exit": 0}
                files_end = None
            else:
                cfg = self.configure(run, i)
                files_end = cfg.pop("files_end")
            rec["configure"] = cfg
            expect = run.get("expect", "ok")
            ok = cfg["exit"] == 0
            if expect == "configure-error":
                if ok:
                    raise SystemExit("run %d: expected a configure error, cmake exited 0" % (i + 1))
            elif not ok:
                tail = "\n".join(cfg.get("tail", [])[-15:])
                raise SystemExit("run %d: cmake failed (exit %d)\n%s" % (i + 1, cfg["exit"], tail))
            if ok and run.get("build_targets", True) is not False:
                b = self.build(run)
                rec["build_result"] = self.norm(b)
                if expect == "build-error":
                    if b["exit"] == 0:
                        raise SystemExit("run %d: expected a build error, build exited 0" % (i + 1))
                elif b["exit"] != 0:
                    raise SystemExit("run %d: build failed\n%s" % (i + 1, "\n".join(b["output"][-20:])))
            after = []
            for cmd in run.get("after", []):
                label, may_fail = None, False
                if isinstance(cmd, dict):
                    label, may_fail, cmd = cmd.get("label"), cmd.get("may_fail", False), cmd["cmd"]
                r = self.run_cmd(cmd, label or " ".join(cmd))
                if r["exit"] != 0 and not may_fail:
                    raise SystemExit("run %d: %s exited %d\n%s" % (i + 1, " ".join(cmd), r["exit"],
                                                                  "\n".join(r["output"][-20:])))
                after.append(self.norm(r))
            if after:
                rec["after"] = after
            record["runs"].append(rec)
        record["blobs"] = self.blobs.data
        return record

def parse_ninja(lines: list[str]) -> list[dict]:
    cmds = []
    pat = re.compile(r"^\[(\d+)/(\d+)\] (.*)$")
    for line in lines:
        m = pat.match(line)
        if m:
            cmds.append({"n": int(m.group(1)), "of": int(m.group(2)), "cmd": m.group(3), "out": []})
        elif cmds and not line.startswith("ninja: "):
            cmds[-1]["out"].append(line)
    return cmds


def cmake_version() -> str:
    out = subprocess.run(["cmake", "--version"], capture_output=True, text=True).stdout
    return out.split("\n")[0].replace("cmake version ", "")


def compiler_version(env: dict) -> str:
    try:
        out = subprocess.run([env.get("CXX", "c++"), "--version"], capture_output=True, text=True,
                             env=env).stdout
    except OSError:
        return ""
    return out.split("\n")[0]


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("example")
    ap.add_argument("-o", "--out", required=True)
    ap.add_argument("--keep", action="store_true", help="keep the work directory and print its path")
    ap.add_argument("--manifest", help="manifest file (default EXAMPLE/sidefx.json)")
    a = ap.parse_args()
    mpath = a.manifest or os.path.join(a.example, "sidefx.json")
    manifest = {}
    if os.path.exists(mpath):
        with open(mpath, encoding="utf-8") as f:
            manifest = json.load(f)
    work = tempfile.mkdtemp(prefix="sidefx-")
    try:
        rec = Recorder(a.example, manifest, work, a.keep).record()
    finally:
        if a.keep:
            print("work directory:", work, file=sys.stderr)
        else:
            shutil.rmtree(work, ignore_errors=True)
    os.makedirs(os.path.dirname(os.path.abspath(a.out)), exist_ok=True)
    with open(a.out, "w", encoding="utf-8") as f:
        json.dump(rec, f, ensure_ascii=False, separators=(",", ":"))
    n = sum(len(r["configure"]["steps"]) for r in rec["runs"])
    print("%s: %d runs, %d steps, %d KB" % (a.out, len(rec["runs"]), n, os.path.getsize(a.out) // 1024))


if __name__ == "__main__":
    main()
