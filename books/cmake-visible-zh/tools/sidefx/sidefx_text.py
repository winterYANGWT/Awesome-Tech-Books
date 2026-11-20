#!/usr/bin/env python3
"""Print a recording as text: one line per step with its side effects.

    python3 -I scripts/sidefx_text.py src/sidefx/ch05/5_2_cache.json [--run N] [--all]

Step numbers are 1-based, the same as the viewer's “第 N 步”. Use this output
(not memory or guesses) when the text says “第 4 步把 X 写进了缓存”.
--all also prints the generate phase (files, final target flags with their origin),
the build commands and the after-commands.
"""
from __future__ import annotations

import argparse
import json
import re
import sys


def short(v, n=70):
    v = str(v)
    return v if len(v) <= n else v[: n - 1] + "…"


def builtin(name: str) -> bool:
    return name.startswith(("CMAKE_", "_")) or name in ("ARGC", "ARGV", "ARGN") or name.startswith("ARGV")


def fx_text(fx: dict) -> list[str]:
    out = []
    for e in fx.get("entered", []):
        sc = e["scope"]
        out.append("进入 %s（%s，经 %s）%s" % (e["file"], ("函数 " + sc["name"]) if sc["kind"] == "function" else ("目录 " + sc["name"]),
                                       e.get("via") or "-", "；新作用域里的变化：" + ", ".join(
            "%s=%s" % (c["name"], short(c.get("new", ""), 30)) for c in e.get("changes", []) if not c["name"].startswith("CMAKE_CURRENT_")) if e.get("changes") else ""))
    for d in fx.get("dropped", []):
        out.append("离开%s %s：消失 %s" % ("函数" if d["scope"]["kind"] == "function" else "目录", d["scope"]["name"],
                                      ", ".join(n for n in d["names"] if not n.startswith("CMAKE_CURRENT_")) or "-"))
    for v in fx.get("vars", []):
        ch = [c for c in v["changes"] if c["name"] not in ("CMAKE_PARENT_LIST_FILE", "CMAKE_CURRENT_LIST_LINE")]
        if not ch:
            continue
        user = [c for c in ch if not builtin(c["name"])]
        n_builtin = len(ch) - len(user)
        sc = v["scope"]
        where = ("函数 " + sc["name"]) if sc["kind"] == "function" else ("目录 " + sc["name"])
        if not v.get("top"):
            where += "（外层作用域）"
        items = []
        for c in user[:12]:
            if c["op"] == "add":
                items.append("+%s=%s" % (c["name"], short(c["new"], 40)))
            elif c["op"] == "del":
                items.append("-%s" % c["name"])
            else:
                items.append("~%s: %s → %s" % (c["name"], short(c["old"], 30), short(c["new"], 40)))
        if len(user) > 12:
            items.append("…另 %d 个" % (len(user) - 12))
        if n_builtin:
            items.append("内置变量 %d 个（%s）" % (n_builtin, ", ".join(c["name"] for c in ch if builtin(c["name"]))[:80]))
        out.append("变量[%s] %s" % (where, "; ".join(items)))
    if fx.get("cache"):
        items = []
        for c in fx["cache"]:
            if c["op"] == "add":
                items.append("+%s:%s=%s" % (c["name"], c["new"]["type"], short(c["new"]["value"], 40)))
            elif c["op"] == "del":
                items.append("-%s" % c["name"])
            else:
                parts = []
                if c["old"]["value"] != c["new"]["value"]:
                    parts.append("%s → %s" % (short(c["old"]["value"], 30), short(c["new"]["value"], 30)))
                if c["old"]["type"] != c["new"]["type"]:
                    parts.append("类型 %s → %s" % (c["old"]["type"], c["new"]["type"]))
                if c["old"].get("help") != c["new"].get("help"):
                    parts.append("说明改了")
                items.append("~%s: %s" % (c["name"], ", ".join(parts)))
        user = [i for i in items if not i[1:].startswith("CMAKE_")]
        n = len(items) - len(user)
        out.append("缓存 " + "; ".join(user[:10]) + (("; 另 %d 个 CMAKE_* 项" % n) if n else ""))
    for t in fx.get("targets", []):
        if t["op"] == "add":
            props = {k: v for k, v in t.get("props", {}).items() if k.startswith("@") and v}
            out.append("新目标 %s %s%s %s" % (t["name"], t["type"], "（导入）" if t.get("imported") else "",
                                           " ".join("%s=%s" % (k[1:], short(v, 40)) for k, v in props.items())))
        elif t["op"] == "set":
            out.append("目标 %s：%s" % (t["name"], "; ".join(
                "%s%s %s" % ({"add": "+", "del": "-", "set": "~"}[c["op"]], c["name"].lstrip("@"),
                             short(c.get("new", ""), 60)) for c in t["changes"])))
    files = [f for f in fx.get("files", []) if "/CMakeFiles/" not in f["path"]]
    internal = len(fx.get("files", [])) - len(files)
    if files or internal:
        out.append("文件 " + ", ".join("%s%s(%dB)" % ({"add": "+", "mod": "~", "del": "-"}[f["op"]], f["path"], f.get("size", 0))
                                       for f in files[:10]) + ((" + CMakeFiles/ 下 %d 个" % internal) if internal else ""))
    if fx.get("out") and any(fx["out"]):
        out.append("输出 " + " ⏎ ".join(l for l in fx["out"] if l)[:300])
    return out


USAGE_PROP = {"-I": ["INTERFACE_INCLUDE_DIRECTORIES", "INTERFACE_SYSTEM_INCLUDE_DIRECTORIES"],
              "-D": ["INTERFACE_COMPILE_DEFINITIONS"], "": ["INTERFACE_COMPILE_OPTIONS"],
              "link": ["INTERFACE_LINK_LIBRARIES", "INTERFACE_LINK_OPTIONS"]}


def final_targets(conf: dict) -> dict:
    """Target table after configuration, rebuilt from the steps (same rules as the viewer)."""
    tg = {}
    for n, t in (conf.get("state0", {}).get("targets") or {}).items():
        tg[n] = {"props": dict(t.get("props", {}))}
    for s in conf.get("steps", []):
        for t in s.get("fx", {}).get("targets", []):
            if t["op"] == "add":
                tg[t["name"]] = {"props": dict(t.get("props", {}))}
            elif t["op"] == "set" and t["name"] in tg:
                for c in t["changes"]:
                    if c["op"] == "del":
                        tg[t["name"]]["props"].pop(c["name"], None)
                    else:
                        tg[t["name"]]["props"][c["name"]] = c["new"]
    return tg


def usage_via(tg: dict, own: str, kind: str, value: str, bt: list) -> str:
    """'来自 X 的 PROP' when a value reached `own` through target_link_libraries."""
    if not bt or bt[0].get("cmd") != "target_link_libraries" or kind not in USAGE_PROP:
        return ""
    queue, seen, order = [own], set(), []
    while queue:
        n = queue.pop(0)
        if n in seen or n not in tg:
            continue
        seen.add(n)
        if n != own:
            order.append(n)
        p = tg[n]["props"]
        libs = (p.get("@LinkImplementations", "") + ";" + p.get("INTERFACE_LINK_LIBRARIES", "")).split(";")
        for x in libs:
            x = re.sub(r"^\$<LINK_ONLY:(.*)>$", r"\1", x)
            if x:
                queue.append(x)
    for dep in order:
        for prop in USAGE_PROP[kind]:
            if value and value in tg[dep]["props"].get(prop, ""):
                return "  · 来自 %s 的 %s" % (dep, prop)
    return ""


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("recording")
    ap.add_argument("--run", type=int)
    ap.add_argument("--all", action="store_true")
    a = ap.parse_args()
    with open(a.recording, encoding="utf-8") as f:
        rec = json.load(f)
    print("# %s   (%s, %s)" % (rec.get("title", ""), rec.get("cmake"), rec.get("compiler")))
    for i, r in enumerate(rec["runs"], 1):
        if a.run and a.run != i:
            continue
        c = r["configure"]
        print("\n## 运行 %d：%s   $ %s   退出码 %s" % (i, r.get("label", ""), " ".join(c.get("argv", [])), c.get("exit")))
        if r.get("replaced"):
            print("   （运行前替换了 %s）" % ", ".join(r["replaced"]))
        for k, s in enumerate(c.get("steps", []), 1):
            src = s["file"] + ":" + str(s["line"])
            cmd = "（文件结束）" if s.get("sentinel") else "%s(%s)" % (s["cmd"], short(" ".join(s.get("args", [])), 60))
            inner = "  [模块里执行了 %d 条命令]" % s["inner"] if s.get("inner") else ""
            print("%3d %-28s %s%s" % (k, src, cmd, inner))
            for line in fx_text(s.get("fx", {})):
                print("      " + line)
        if c.get("tail"):
            print("   配置结束后的输出：" + " ⏎ ".join(l for l in c["tail"] if l)[:400])
        if not a.all:
            continue
        g = c.get("generate", {}).get("files", [])
        print("   生成阶段写的文件：" + ", ".join(f["path"] for f in g if "/CMakeFiles/" not in f["path"]))
        tg = final_targets(c)
        for t in (c.get("codemodel") or {}).get("targets", []):
            print("   目标 %s (%s)" % (t["name"], t["type"]))
            for gr in t.get("groups", []):
                for kind, key in (("-I", "includes"), ("-D", "defines"), ("", "flags")):
                    for x in gr.get(key, []):
                        bts = [b for b in x.get("bt", []) if b.get("line")]
                        bt = " ← ".join("%s:%s %s" % (b["file"], b["line"], b.get("cmd", "")) for b in bts)
                        via = usage_via(tg, t["name"], kind, x["v"].strip(), bts)
                        print("      %s%s  ← %s%s" % (kind, x["v"].strip(), bt or "CMake 自己加的", via))
            for x in t.get("link", []):
                if x["v"].strip():
                    bt = " ← ".join("%s:%s %s" % (b["file"], b["line"], b.get("cmd", "")) for b in x.get("bt", []) if b.get("line"))
                    print("      链接 %s [%s] ← %s" % (x["v"].strip(), x["role"], bt or "CMake"))
        b = r.get("build_result")
        if b:
            print("   构建：退出码 %d，%d 条命令" % (b["exit"], len(b.get("commands", []))))
            for cm in b.get("commands", []):
                print("      [%d/%d] %s" % (cm["n"], cm["of"], short(cm["cmd"], 200)))
        for x in r.get("after", []):
            print("   $ %s → 退出码 %d" % (" ".join(x["argv"]), x["exit"]))
            for line in x.get("output", [])[:15]:
                print("      | " + line)
            fl = [f["path"] for f in x.get("files", []) if "/CMakeFiles/" not in f["path"] and not f["path"].endswith((".ninja_log", ".ninja_deps"))]
            if fl:
                print("      改动文件：" + ", ".join(fl[:20]))


if __name__ == "__main__":
    main()
