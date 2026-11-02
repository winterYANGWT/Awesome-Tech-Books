#!/usr/bin/env python3
"""Inject the shared reader assets into every built mdBook content page.

Usage: python3 scripts/inject-reader.py <site-dir>

Only *.html files containing id="mdbook-content" are touched, and print.html
and 404.html are skipped, so redirect stubs, toc.html and sample HTML shipped
with a book are left alone. reader/ itself is not scanned.
"""

import os
import re
import sys
from pathlib import Path

MARKER = 'id="mdbook-content"'
HEAD_END = re.compile(r"[ \t]*</head>")
SKIP_NAMES = {"print.html", "404.html"}
SNIPPET = (
    '        <link rel="stylesheet" href="{rel}reader/reader.css">\n'
    '        <script src="{rel}reader/reader.js"></script>\n'
)


def inject(page: Path, site: Path) -> bool:
    """Add the two tags to one page; return whether it was a reader page."""
    if page.name in SKIP_NAMES:
        return False

    data = page.read_bytes()
    text = data.decode("utf-8")
    if MARKER not in text:
        return False

    # Match the leading indent too, so the snippet gets head-child indent.
    head_end = HEAD_END.search(text)
    if head_end is None:
        raise ValueError(f"{page}: has {MARKER} but no </head>")

    depth = len(page.parent.relative_to(site).parts)
    rel = "../" * depth
    at = head_end.start()
    text = text[:at] + SNIPPET.format(rel=rel) + text[at:]
    page.write_bytes(text.encode("utf-8"))
    return True


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(f"usage: {argv[0]} <site-dir>", file=sys.stderr)
        return 2

    site = Path(argv[1])
    if not site.is_dir():
        print(f"error: not a directory: {site}", file=sys.stderr)
        return 2

    failures = []
    for book in sorted(p for p in site.iterdir() if p.is_dir()):
        if book.name == "reader":
            continue

        count = 0
        failed = False
        for root, dirs, files in os.walk(book):
            dirs.sort()
            for name in sorted(files):
                if not name.endswith(".html"):
                    continue
                try:
                    if inject(Path(root) / name, site):
                        count += 1
                except (OSError, UnicodeDecodeError, ValueError) as exc:
                    print(f"error: {exc}", file=sys.stderr)
                    failed = True

        print(f"==> {book.name}: injected into {count} page(s)")
        if count == 0:
            print(
                f"error: {book.name}: no reader pages injected; mdBook may "
                f"have changed {MARKER}",
                file=sys.stderr,
            )
            failed = True
        if failed:
            failures.append(book.name)

    if failures:
        print(
            "error: reader injection failed for: " + ", ".join(failures),
            file=sys.stderr,
        )
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
