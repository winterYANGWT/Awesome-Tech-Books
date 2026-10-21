#!/usr/bin/env bash
# Assemble the whole GitHub Pages site into one directory.
# Books under books/<name>/ are mdBook sources and are built to <name>/;
# everything else (index.html, rust-book-zh/) is copied as committed.
# Used by .github/workflows/pages.yml; also works locally for a preview.
#   usage: scripts/build-site.sh [out-dir]   (default _site; needs mdbook on PATH)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${1:-$ROOT/_site}"
case "$OUT" in /*) ;; *) OUT="$PWD/$OUT" ;; esac

# _site is ours to replace; any other directory must not exist yet.
if [ "$OUT" = "$ROOT/_site" ]; then
    rm -rf "$OUT"
elif [ -e "$OUT" ]; then
    echo "$OUT already exists; remove it or pick another directory" >&2
    exit 1
fi
mkdir -p "$OUT"

cp "$ROOT/index.html" "$OUT/"
cp -r "$ROOT/rust-book-zh" "$OUT/"
for book in "$ROOT"/books/*/; do
    name="$(basename "$book")"
    mdbook build "$book" -d "$OUT/$name"
    # The licence texts travel with the book, as its copyright page promises.
    cp "$book"LICENSE* "$OUT/$name/"
done

echo "==> site written to $OUT ($(find "$OUT" -type f | wc -l) files, $(du -sh "$OUT" | cut -f1))"
