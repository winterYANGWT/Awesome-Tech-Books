#!/usr/bin/env python3
"""mdBook 预处理器：把 {{#sidefx chNN/名字 [选项]}} 换成副作用查看器的启动条。

    {{#sidefx ch05/5_2_cache}}
    {{#sidefx ch05/5_2_cache run=2 phase=generate step=4 label="看第二次配置"}}

录制文件是 src/sidefx/chNN/名字.json（scripts/check_examples.py 生成）。启动条紧跟在代码块
后面时贴在代码块下沿，否则单独成块。启动条上的数字从录制里统计：配置几步、新建几个目标、
用户自己的缓存项、写了几个文件、构建几条命令。录制不存在时构建失败。
"""
from __future__ import annotations

import html
import json
import os
import re
import shlex
import sys

DIRECTIVE = re.compile(r"\{\{#sidefx\s+([^}]*?)\s*\}\}")
FENCE = re.compile(r"^\s*(```+|~~~+)")
ERRORS: list[str] = []


def mask_ranges(text: str) -> list[tuple[int, int]]:
    """Fenced code blocks, so directives shown as code are left alone."""
    ranges, start, fence = [], None, None
    pos = 0
    for line in text.splitlines(keepends=True):
        m = FENCE.match(line)
        if m:
            if fence is None:
                fence, start = m.group(1)[0] * 3, pos
            elif m.group(1).startswith(fence):
                ranges.append((start, pos + len(line)))
                fence = None
        pos += len(line)
    return ranges


def summarize(rec: dict) -> list[str]:
    chips = []
    runs = rec.get("runs", [])
    steps = sum(len(r.get("configure", {}).get("steps", [])) for r in runs)
    if len(runs) > 1:
        chips.append("<b>%d</b> 次运行" % len(runs))
    chips.append("配置 <b>%d</b> 步" % steps)
    targets, cache, files, cmds = set(), set(), set(), 0
    for r in runs:
        for s in r.get("configure", {}).get("steps", []):
            fx = s.get("fx", {})
            for t in fx.get("targets", []):
                if t.get("op") == "add" and not t.get("imported"):
                    targets.add(t["name"])
            for c in fx.get("cache", []):
                if c.get("op") == "add" and not c["name"].startswith(("CMAKE_", "_")) and \
                        c.get("new", {}).get("type") not in ("INTERNAL", "STATIC"):
                    cache.add(c["name"])
            for f in fx.get("files", []):
                if "/CMakeFiles/" not in f["path"] and f.get("op") != "del":
                    files.add(f["path"])
        b = r.get("build_result")
        if b:
            cmds += len(b.get("commands", []))
    if targets:
        chips.append("新目标 <b>%d</b>" % len(targets))
    if cache:
        chips.append("缓存项 <b>%d</b>" % len(cache))
    if files:
        chips.append("配置时写文件 <b>%d</b>" % len(files))
    if cmds:
        chips.append("构建命令 <b>%d</b>" % cmds)
    if any(r.get("configure", {}).get("exit") for r in runs):
        chips.append("<b>配置报错</b>")
    return chips


def render(arg: str, chapter_path: str, src_dir: str, attached: bool) -> str:
    try:
        parts = shlex.split(arg)
    except ValueError as e:
        ERRORS.append("%s: bad {{#sidefx %s}}: %s" % (chapter_path, arg, e))
        return ""
    if not parts:
        ERRORS.append("%s: {{#sidefx}} without a name" % chapter_path)
        return ""
    ref, opts = parts[0], {}
    for p in parts[1:]:
        k, _, v = p.partition("=")
        opts[k] = v
    path = os.path.join(src_dir, "sidefx", ref + ".json")
    if not os.path.exists(path):
        ERRORS.append("%s: recording %s does not exist (run scripts/check_examples.py)" % (chapter_path, path))
        return '<div class="sfx-launch sfx-alone">缺少录制：%s</div>' % html.escape(ref)
    with open(path, encoding="utf-8") as f:
        rec = json.load(f)
    depth = chapter_path.count("/")
    url = "../" * depth + "sidefx/" + ref + ".json"
    attrs = ['class="sfx-launch%s"' % ("" if attached else " sfx-alone"),
             'data-src="%s"' % html.escape(url)]
    if opts.get("run"):
        attrs.append('data-run="%d"' % (int(opts["run"]) - 1))
    if opts.get("phase"):
        attrs.append('data-phase="%s"' % html.escape(opts["phase"]))
    if opts.get("step"):
        attrs.append('data-step="%d"' % (int(opts["step"]) - 1))
    label = opts.get("label") or "▶ 逐行执行，看副作用"
    chips = "".join('<span class="sfx-chip">%s</span>' % c for c in summarize(rec))
    return ('<div %s><button class="sfx-open" type="button">%s</button>%s</div>'
            % (" ".join(attrs), html.escape(label), chips))


def process(content: str, chapter_path: str, src_dir: str) -> str:
    masks = mask_ranges(content)
    out, last = [], 0
    for m in DIRECTIVE.finditer(content):
        if any(a <= m.start() < b for a, b in masks):
            continue
        before = content[:m.start()]
        stripped = before.rstrip("\n")
        prev_line = stripped.rsplit("\n", 1)[-1].strip()
        # 紧跟在代码块的结束行后面（中间最多一个空行）就贴在代码块下沿
        attached = bool(re.fullmatch(r"(`{3,}|~{3,})", prev_line)) and len(before) - len(stripped) <= 2
        out.append(content[last:m.start()])
        out.append(render(m.group(1), chapter_path, src_dir, attached))
        last = m.end()
    out.append(content[last:])
    return "".join(out)


DETAILS_LINE = re.compile(r"^\s*(?:\d+\.\s+)?<details>")
CODE_SPAN = re.compile(r"`([^`\n]+)`")


def escape_details(content: str) -> str:
    """<details> 里的提示是原样 HTML，Markdown 的反引号不起作用：`$<CONFIG>` 会被当成标签。
    把这些行里的反引号代码换成转义过的 <code>。"""
    masks = mask_ranges(content)
    out, pos = [], 0
    for line in content.splitlines(keepends=True):
        if DETAILS_LINE.match(line) and not any(a <= pos < b for a, b in masks):
            line = CODE_SPAN.sub(lambda m: "<code>" + html.escape(m.group(1), quote=False) + "</code>", line)
        out.append(line)
        pos += len(line)
    return "".join(out)


def walk(items, src_dir):
    for it in items:
        ch = it.get("Chapter") if isinstance(it, dict) else None
        if not ch:
            continue
        if ch.get("path"):
            ch["content"] = escape_details(process(ch["content"], ch["path"], src_dir))
        walk(ch.get("sub_items", []), src_dir)


def main() -> None:
    if len(sys.argv) > 1 and sys.argv[1] == "supports":
        sys.exit(0)
    context, book = json.load(sys.stdin)
    src_dir = os.path.join(context["root"], context["config"]["book"].get("src", "src"))
    walk(book.get("items", book.get("sections", [])), src_dir)
    if ERRORS:
        for e in ERRORS:
            print("ERROR sidefx: " + e, file=sys.stderr)
    json.dump(book, sys.stdout, ensure_ascii=False)


if __name__ == "__main__":
    main()
