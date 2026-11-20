#!/usr/bin/env python3
"""Record a CMake project and open the side-effect viewer on it.

    python3 sidefx.py PROJECT_DIR                 # writes PROJECT_DIR.sidefx.html next to it
    python3 sidefx.py PROJECT_DIR -o view.html    # choose the output file
    python3 sidefx.py PROJECT_DIR --serve 3001    # also serve it on http://0.0.0.0:3001/
    python3 sidefx.py --from-json rec.json -o view.html   # only build the page

Extra cmake arguments go after `--`:

    python3 sidefx.py ~/code/mylib -- -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug

The project is copied to a temporary directory first, so your source tree and
build directories are not touched. A sidefx.json in the project (see record.py)
describes several runs; without it there is one run: configure, then build.
"""
from __future__ import annotations

import argparse
import http.server
import json
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
THEME = os.path.normpath(os.path.join(HERE, "..", "..", "theme"))

PAGE = """<!doctype html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{title}</title>
<style>
html, body {{ margin: 0; padding: 0; background: var(--sfx-bg, #fff); }}
body {{ font-family: -apple-system, "Segoe UI", "PingFang SC", "Noto Sans CJK SC", "Microsoft YaHei", sans-serif; }}
{css}
</style>
</head>
<body>
<script>window.SIDEFX_DATA = {data};</script>
<script>
{js}
</script>
</body>
</html>
"""


def build_page(rec: dict, out: str) -> None:
    with open(os.path.join(THEME, "sidefx.css"), encoding="utf-8") as f:
        css = f.read()
    with open(os.path.join(THEME, "sidefx.js"), encoding="utf-8") as f:
        js = f.read()
    data = json.dumps(rec, ensure_ascii=False, separators=(",", ":")).replace("</", "<\\/")
    title = (rec.get("title") or "CMake 副作用").replace("<", "&lt;")
    with open(out, "w", encoding="utf-8") as f:
        f.write(PAGE.format(title=title, css=css, js=js, data=data))


def main() -> None:
    argv = sys.argv[1:]
    extra: list[str] = []
    if "--" in argv:
        i = argv.index("--")
        argv, extra = argv[:i], argv[i + 1:]
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("project", nargs="?")
    ap.add_argument("-o", "--out")
    ap.add_argument("--from-json")
    ap.add_argument("--serve", type=int, metavar="PORT")
    ap.add_argument("--no-build", action="store_true", help="configure only")
    a = ap.parse_args(argv)
    if a.from_json:
        with open(a.from_json, encoding="utf-8") as f:
            rec = json.load(f)
        out = a.out or os.path.splitext(a.from_json)[0] + ".html"
    else:
        if not a.project:
            ap.error("PROJECT_DIR is required")
        project = os.path.abspath(a.project)
        out = a.out or project.rstrip("/") + ".sidefx.html"
        with tempfile.TemporaryDirectory(prefix="sidefx-rec-") as tmp:
            jpath = os.path.join(tmp, "rec.json")
            cmd = [sys.executable, "-I", os.path.join(HERE, "record.py"), project, "-o", jpath]
            manifest = os.path.join(project, "sidefx.json")
            if extra or a.no_build or not os.path.exists(manifest):
                m = {"title": os.path.basename(project.rstrip("/")),
                     "runs": [{"args": extra, "build_targets": not a.no_build}]}
                mpath = os.path.join(tmp, "manifest.json")
                with open(mpath, "w", encoding="utf-8") as f:
                    json.dump(m, f, ensure_ascii=False)
                cmd += ["--manifest", mpath]
            subprocess.run(cmd, check=True)
            with open(jpath, encoding="utf-8") as f:
                rec = json.load(f)
    build_page(rec, out)
    print("viewer:", out)
    if a.serve:
        directory, name = os.path.split(os.path.abspath(out))
        handler = lambda *x, **k: http.server.SimpleHTTPRequestHandler(*x, directory=directory, **k)
        srv = http.server.ThreadingHTTPServer(("0.0.0.0", a.serve), handler)
        print("open http://127.0.0.1:%d/%s  (Ctrl-C to stop)" % (a.serve, name))
        try:
            srv.serve_forever()
        except KeyboardInterrupt:
            pass


if __name__ == "__main__":
    main()
