#!/usr/bin/env python3
"""mdBook 预处理器：把 {{#fig ...}} 换成图，把 {{#figref ...}} 换成链接。

只使用 Python 标准库。图的几何由 theme/fig.js 在浏览器里算，这里只负责
读取 JSON、给图编号、生成无障碍的 HTML 外壳和给阅读器/打印用的文字版。
"""
from __future__ import annotations

import csv
import html
import json
import os
import re
import sys

FIG_RE = re.compile(r"\{\{#fig\s+([^\s{}]+)\s*\}\}")
FIGREF_RE = re.compile(r"\{\{#figref\s+([^\s{}]+)\s*\}\}")

KINDS = {"seq", "grid", "flow", "chart"}


class FigError(Exception):
    """带章节和行号的错误。"""


def escape(text: object) -> str:
    return html.escape(str(text), quote=False)


def escape_attr(text: object) -> str:
    return html.escape(str(text), quote=True)


def rich(text: object) -> str:
    """把反引号包起来的文字变成 <code>，其余做 HTML 转义。"""
    s = str(text)
    out: list[str] = []
    i = 0
    while i < len(s):
        if s[i] == "`":
            j = s.find("`", i + 1)
            if j > i + 1:
                out.append("<code>" + escape(s[i + 1:j]) + "</code>")
                i = j + 1
                continue
        out.append(escape(s[i]))
        i += 1
    return "".join(out)


# ---------------------------------------------------------------------------
# 找出代码块和行内代码，避免替换其中的指令
# ---------------------------------------------------------------------------

def _mask(chars: list[str], start: int, end: int) -> None:
    for k in range(start, end):
        if chars[k] != "\n":
            chars[k] = " "


def _code_close(body: str, start: int, run: int) -> int:
    i = start
    while i < len(body):
        if body[i] != "`":
            i += 1
            continue
        j = i
        while j < len(body) and body[j] == "`":
            j += 1
        if j - i == run:
            return i
        i = j
    return -1

def mask_code(text: str) -> str:
    """把围栏代码块和行内代码里的字符换成空格，保留换行和长度。"""
    chars = list(text)
    lines = text.splitlines(keepends=True)
    starts: list[int] = []
    pos = 0
    for line in lines:
        starts.append(pos)
        pos += len(line)
    starts.append(pos)

    fenced: set[int] = set()
    i = 0
    while i < len(lines):
        body = lines[i].rstrip("\r\n")
        opening = re.match(r"^ {0,3}(`{3,}|~{3,})(.*)$", body)
        if not opening:
            i += 1
            continue
        marker, info = opening.group(1), opening.group(2)
        if marker[0] == "`" and "`" in info:
            i += 1
            continue
        closing = re.compile(
            r"^ {0,3}" + re.escape(marker[0]) + "{" + str(len(marker)) + r",}\s*$"
        )
        j = i + 1
        while j < len(lines) and not closing.match(lines[j].rstrip("\r\n")):
            j += 1
        stop = j + 1 if j < len(lines) else len(lines)
        _mask(chars, starts[i], starts[stop])
        for k in range(i, stop):
            fenced.add(k)
        i = stop

    # 行内代码（围栏外）
    for lineno, line in enumerate(lines):
        if lineno in fenced:
            continue
        base = starts[lineno]
        k = 0
        body = line.rstrip("\r\n")
        while k < len(body):
            if body[k] != "`":
                k += 1
                continue
            run = 1
            while k + run < len(body) and body[k + run] == "`":
                run += 1
            close = _code_close(body, k + run, run)
            if close < 0:
                k += run
                continue
            _mask(chars, base + k, base + close + run)
            k = close + run
    return "".join(chars)


def line_of(text: str, pos: int) -> int:
    return text.count("\n", 0, pos) + 1


def chapter_number(path: str) -> str | None:
    base = os.path.basename(path or "")
    m = re.fullmatch(r"ch(\d+)\.md", base)
    if m:
        return str(int(m.group(1)))
    if re.match(r"appendix-a.*\.md$", base):
        return "A"
    return None


def figure_id(ref: str) -> str:
    safe = re.sub(r"[^A-Za-z0-9_-]+", "-", ref).strip("-")
    return "fig-" + (safe or "figure")


def link_path(from_path: str, to_path: str) -> str:
    start = os.path.dirname(from_path or "")
    rel = os.path.relpath(to_path or "", start) if start else (to_path or "")
    return rel.replace(os.sep, "/")


def iter_chapters(items):
    for item in items:
        if "Chapter" in item:
            chapter = item["Chapter"]
            yield chapter
            yield from iter_chapters(chapter.get("sub_items", []))
        elif "PartTitle" in item:
            yield from iter_chapters(item.get("sub_items", []))


# ---------------------------------------------------------------------------
# 文字版
# ---------------------------------------------------------------------------

def static_figure(spec: dict) -> bool:
    if spec.get("kind") == "chart":
        return True
    steps = spec.get("steps") or []
    return bool(spec.get("static")) or len(steps) <= 1


def text_version(spec: dict) -> str:
    if spec.get("kind") == "chart":
        rows = spec.get("rows") or []
        if not rows:
            return "<p>" + rich(spec.get("alt", "")) + "</p>"
        columns = list(rows[0].keys())
        head = "".join("<th>" + escape(c) + "</th>" for c in columns)
        body = []
        for row in rows:
            body.append(
                "<tr>" + "".join(
                    "<td>" + escape(row.get(c, "")) + "</td>" for c in columns
                ) + "</tr>"
            )
        return (
            "<table><thead><tr>" + head + "</tr></thead><tbody>"
            + "".join(body) + "</tbody></table>"
        )
    if static_figure(spec):
        return "<p>" + rich(spec.get("alt", "")) + "</p>"
    items = []
    for step in spec.get("steps") or []:
        items.append("<li>" + rich(step.get("note", "")) + "</li>")
    return "<ol>" + "".join(items) + "</ol>"


def figure_html(spec: dict, ref: str, num: str | None, title: str) -> str:
    fid = figure_id(ref)
    kind = spec.get("kind", "")
    caption = ""
    if num is not None:
        caption = '<span class="ccfig-num">图 ' + escape(num) + "</span> "
    caption += escape(title)
    return (
        '<figure class="ccfig" id="' + escape_attr(fid) + '" data-kind="' + escape_attr(kind) + '">'
        '<div class="ccfig-stage" tabindex="0" role="img" aria-label="'
        + escape_attr(spec.get("alt", "")) + '"></div>'
        '<script type="application/json" class="ccfig-spec">'
        + spec["_json"] + "</script>"
        "<figcaption>" + caption + "</figcaption>"
        '<details class="ccfig-text"><summary>文字版</summary>'
        + text_version(spec) + "</details>"
        "</figure>"
    )


# ---------------------------------------------------------------------------
# 读图和数据
# ---------------------------------------------------------------------------

def load_json(path: str, where: str) -> dict:
    if not os.path.isfile(path):
        raise FigError(where + ": figure file not found: " + path)
    try:
        with open(path, "r", encoding="utf-8") as fh:
            spec = json.load(fh)
    except json.JSONDecodeError as exc:
        raise FigError(where + ": invalid JSON in " + path + ": " + str(exc))
    except OSError as exc:
        raise FigError(where + ": cannot read " + path + ": " + str(exc))
    if not isinstance(spec, dict):
        raise FigError(where + ": " + path + " must contain a JSON object")
    kind = spec.get("kind")
    if kind not in KINDS:
        raise FigError(where + ": " + path + ": unknown kind " + repr(kind))
    for key in ("title", "alt"):
        if not isinstance(spec.get(key), str) or not spec[key].strip():
            raise FigError(where + ": " + path + ": missing or empty " + key)
    if kind != "chart":
        if not isinstance(spec.get("steps"), list) or not spec["steps"]:
            raise FigError(where + ": " + path + ": steps must be a non-empty array")
        for index, step in enumerate(spec["steps"]):
            if not isinstance(step, dict):
                raise FigError(where + ": " + path + ": steps[" + str(index) + "] is not an object")
            if not isinstance(step.get("note"), str) or not step["note"].strip():
                raise FigError(where + ": " + path + ": steps[" + str(index) + "] has no note")
    else:
        if not isinstance(spec.get("data"), str) or not spec["data"].strip():
            raise FigError(where + ": " + path + ": chart has no data file")
    return spec


def load_csv(path: str, where: str) -> list[dict]:
    if not os.path.isfile(path):
        raise FigError(where + ": chart data file not found: " + path)
    try:
        with open(path, "r", encoding="utf-8-sig", newline="") as fh:
            reader = csv.DictReader(fh)
            if not reader.fieldnames:
                raise FigError(where + ": chart data file has no header: " + path)
            rows = []
            for row in reader:
                if None in row:
                    raise FigError(where + ": malformed CSV row in " + path)
                rows.append({key: ("" if value is None else value) for key, value in row.items()})
    except OSError as exc:
        raise FigError(where + ": cannot read " + path + ": " + exc)
    if not rows:
        raise FigError(where + ": chart data file has no rows: " + path)
    return rows


def prepare_spec(spec: dict, ref: str, src_dir: str, where: str) -> dict:
    if spec.get("kind") == "chart":
        data_path = os.path.normpath(os.path.join(src_dir, spec["data"]))
        spec["rows"] = load_csv(data_path, where)
    normalized = json.dumps(spec, ensure_ascii=False, separators=(",", ":"))
    spec["_json"] = normalized.replace("</", "<\\/")
    return spec


# ---------------------------------------------------------------------------
# 主流程
# ---------------------------------------------------------------------------

def run(root: str, config: dict, book: dict) -> None:
    src = config.get("book", {}).get("src", "src")
    src_dir = os.path.join(root, src)
    figs_dir = os.path.join(src_dir, "figs")
    chapters = list(iter_chapters(book.get("items", [])))

    # 第一遍：编号、读入、检查重复定义。
    fig_by_ref: dict[str, dict] = {}
    per_chapter: dict[int, list[dict]] = {}
    for index, chapter in enumerate(chapters):
        path = chapter.get("path") or chapter.get("source_path") or ""
        content = chapter.get("content", "")
        masked = mask_code(content)
        number = chapter_number(path)
        counter = 0
        found = []
        for match in FIG_RE.finditer(masked):
            pos = match.start()
            line = line_of(masked, pos)
            where = f"{path or chapter.get('name', '?')}:{line}"
            line_start = masked.rfind("\n", 0, pos) + 1
            line_end = masked.find("\n", pos)
            if line_end < 0:
                line_end = len(masked)
            if masked[line_start:line_end].strip() != match.group(0):
                raise FigError(where + ": {{#fig}} must be alone on its line")
            ref = match.group(1)
            if ref in fig_by_ref:
                raise FigError(where + ": figure " + ref + " is used more than once")
            filename = os.path.join(figs_dir, *ref.split("/")) + ".json"
            spec = load_json(filename, where)
            spec = prepare_spec(spec, ref, src_dir, where)
            num = None
            if number is not None:
                counter += 1
                num = number + "-" + str(counter)
            record = {
                "chapter_index": index,
                "chapter_path": path,
                "start": line_start,
                "end": line_end,
                "ref": ref,
                "line": line,
                "num": num,
                "spec": spec,
                "html": figure_html(spec, ref, num, spec["title"]),
            }
            found.append(record)
            fig_by_ref[ref] = record
        per_chapter[index] = found

    # 第二遍：所有 figref 必须指向已存在的图。
    for index, chapter in enumerate(chapters):
        path = chapter.get("path") or chapter.get("source_path") or ""
        content = chapter.get("content", "")
        masked = mask_code(content)
        for match in FIGREF_RE.finditer(masked):
            ref = match.group(1)
            if ref not in fig_by_ref:
                line = line_of(masked, match.start())
                raise FigError(
                    f"{path or chapter.get('name', '?')}:{line}: "
                    + "{{#figref}} points to unknown figure " + ref
                )

    # 第三遍：替换。同一章内从后往前替换，避免位置移动。
    for index, chapter in enumerate(chapters):
        path = chapter.get("path") or chapter.get("source_path") or ""
        content = chapter.get("content", "")
        masked = mask_code(content)
        replacements: list[tuple[int, int, str]] = []
        for record in per_chapter[index]:
            before = content[:record["start"]]
            after = content[record["end"]:]
            prefix = "" if record["start"] == 0 or before.endswith("\n\n") else "\n"
            suffix = "" if record["end"] == len(content) or after.startswith("\n\n") else "\n"
            replacements.append((record["start"], record["end"], prefix + record["html"] + suffix))
        for match in FIGREF_RE.finditer(masked):
            ref = match.group(1)
            target = fig_by_ref[ref]
            if target["num"] is not None:
                label = "图 " + target["num"]
            else:
                label = target["spec"]["title"]
            target_path = target["chapter_path"]
            href = link_path(path, target_path) + "#" + figure_id(ref)
            # 只转义 markdown 链接里真正会破坏语法的字符。
            label_md = label.replace("\\", "\\\\").replace("]", "\\]")
            href_md = href.replace(" ", "%20").replace(")", "%29")
            replacements.append((match.start(), match.end(), f"[{label_md}]({href_md})"))
        replacements.sort(key=lambda item: item[0], reverse=True)
        for start, end, replacement in replacements:
            content = content[:start] + replacement + content[end:]
        chapter["content"] = content


def main() -> int:
    if len(sys.argv) > 1 and sys.argv[1] == "supports":
        return 0 if len(sys.argv) > 2 and sys.argv[2] == "html" else 1
    try:
        context, book = json.load(sys.stdin)
    except Exception as exc:
        print(f"fig_preprocess: cannot read mdBook JSON: {exc}", file=sys.stderr)
        return 1
    root = context.get("root", ".")
    try:
        run(root, context.get("config", {}), book)
    except FigError as exc:
        print(str(exc), file=sys.stderr)
        return 1
    json.dump(book, sys.stdout, ensure_ascii=False)
    return 0


if __name__ == "__main__":
    sys.exit(main())
