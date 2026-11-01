#!/usr/bin/env python3
"""mdBook preprocessor: hand every TeX formula to MathJax unchanged.

The chapters use the standard TeX delimiters ``\\(...\\)``, ``\\[...\\]`` and
(where present) ``$$...$$``.  It is not enough to double the four delimiter
backslashes: CommonMark would still interpret characters inside the formula
(``_`` and ``*`` as emphasis, ``\\\\`` as a line break, ``\\{`` and ``\\,`` as
escapes, ``'`` as smart punctuation, ``<`` as HTML, ...), so the text MathJax
receives would no longer be the source TeX.

This preprocessor therefore backslash-escapes *every* ASCII punctuation
character inside a formula.  In CommonMark a backslash before ASCII punctuation
always yields that literal character, while a backslash itself is written as
``\\\\``.  The rendered text is consequently byte-for-byte the source TeX,
including the delimiters.  Fenced code blocks and inline code spans are skipped.

The transformation lives in a preprocessor rather than in the chapters so the
Markdown sources stay readable and the same conversion is used by
``scripts/build.sh`` and the GitHub Pages build.
"""
import json
import re
import string
import sys

# ASCII punctuation is exactly the set CommonMark lets a backslash escape.
PUNCTUATION = frozenset(string.punctuation)

# Older chapters (ch01) already spell the delimiters as ``\\(``/``\\[``, i.e.
# one pre-escaped layer.  They are recognised too and normalised before being
# protected, so all chapters reach MathJax with the same single-backslash TeX.
OPENERS = (
    ("\\\\(", "\\\\)"),
    ("\\\\[", "\\\\]"),
    ("\\(", "\\)"),
    ("\\[", "\\]"),
    ("$$", "$$"),
)
LEGACY_PREFIXES = ("\\\\(", "\\\\[")


def _fenced_ranges(text):
    """Return [(start, end)] character ranges occupied by fenced code blocks.

    CommonMark fences are at most three spaces in, opened by at least three
    backticks or tildes, and closed by a run of the same character at least as
    long as the opener.  An unclosed fence runs to the end of the document.
    """
    lines = text.splitlines(keepends=True)
    starts, pos = [], 0
    for line in lines:
        starts.append(pos)
        pos += len(line)
    starts.append(pos)
    ranges = []
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
            r"^ {0,3}" + re.escape(marker[0]) + r"{" + str(len(marker)) + r",}\s*$"
        )
        j = i + 1
        while j < len(lines) and not closing.match(lines[j].rstrip("\r\n")):
            j += 1
        if j < len(lines):
            ranges.append((starts[i], starts[j + 1]))
            i = j + 1
        else:
            ranges.append((starts[i], len(text)))
            break
    return ranges


def _escaped(text, index):
    """True if text[index] is preceded by an odd number of backslashes."""
    count = 0
    index -= 1
    while index >= 0 and text[index] == "\\":
        count += 1
        index -= 1
    return count % 2 == 1


def _code_close(text, start, run, limit):
    """End offset of an inline code span starting at ``start``, or None.

    A span opened by N backticks is closed by the next run of exactly N
    backticks; longer runs are literal content.
    """
    i = start
    while i < limit:
        if text[i] != "`":
            i += 1
            continue
        j = i
        while j < limit and text[j] == "`":
            j += 1
        if j - i == run:
            return j
        i = j
    return None


def find_math_spans(text):
    """Return [(start, end, opener, closer)] for formulas outside code.

    ``start``/``end`` cover the delimiters as well as the formula body, so
    ``text[start:end]`` is exactly the source TeX interval.
    """
    fences = _fenced_ranges(text)
    spans = []
    fence_index = 0
    i, n = 0, len(text)
    while i < n:
        while fence_index < len(fences) and i >= fences[fence_index][0]:
            i = fences[fence_index][1]
            fence_index += 1
        if i >= n:
            break
        limit = fences[fence_index][0] if fence_index < len(fences) else n
        if text[i] == "`":
            j = i
            while j < limit and text[j] == "`":
                j += 1
            close = _code_close(text, j, j - i, limit)
            i = close if close is not None else j
            continue
        matched = False
        for opener, closer in OPENERS:
            if not text.startswith(opener, i) or _escaped(text, i):
                continue
            end = text.find(closer, i + len(opener))
            if end < 0:
                continue
            spans.append((i, end + len(closer), opener, closer))
            i = end + len(closer)
            matched = True
            break
        if not matched:
            i += 1
    return spans


def _strip_legacy(segment):
    """Remove one CommonMark escape layer from a pre-escaped formula."""
    return re.sub(r"\\([!\"#$%&'()*+,\-./:;<=>?@\[\\\]^_`{|}~])", r"\1", segment)


def source_tex(segment):
    """Return the TeX a source formula stands for.

    Normal formulas use single-backslash delimiters and are returned verbatim;
    a legacy pre-escaped formula (``\\\\(...\\\\)``) is unescaped one layer.
    """
    if segment.startswith(LEGACY_PREFIXES):
        return _strip_legacy(segment)
    return segment


def protect_math(segment):
    """Backslash-escape every ASCII punctuation character in ``segment``."""
    segment = source_tex(segment)
    return "".join("\\" + ch if ch in PUNCTUATION else ch for ch in segment)


def fix_markdown(text):
    """Protect every formula interval, leaving code untouched."""
    spans = find_math_spans(text)
    if not spans:
        return text
    out, last = [], 0
    for start, end, _opener, _closer in spans:
        out.append(text[last:start])
        out.append(protect_math(text[start:end]))
        last = end
    out.append(text[last:])
    return "".join(out)


def fix_line(line):
    """Backward-compatible helper used by older tests."""
    return fix_markdown(line)


def walk(items):
    for item in items:
        if "Chapter" in item:
            chapter = item["Chapter"]
            chapter["content"] = fix_markdown(chapter.get("content", ""))
            walk(chapter.get("sub_items", []))
        elif "PartTitle" in item or "Separator" in item:
            continue


def main():
    # mdBook first calls `<command> supports <renderer>` and expects exit 0.
    if len(sys.argv) > 1 and sys.argv[1] == "supports":
        return 0
    try:
        context, book = json.load(sys.stdin)
    except Exception as exc:  # mdBook shows this on stderr and fails the build
        print(f"mathjax_preprocess: cannot read mdBook JSON: {exc}", file=sys.stderr)
        return 1
    walk(book.get("items", []))
    json.dump(book, sys.stdout, ensure_ascii=False)
    return 0


if __name__ == "__main__":
    sys.exit(main())
