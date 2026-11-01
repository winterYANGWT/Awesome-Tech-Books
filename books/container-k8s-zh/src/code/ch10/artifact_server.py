#!/usr/bin/env python3
"""第 10 章的私有制品库：只服务 /srv 下的文件，请求必须带构建 token。"""

import functools
import http.server
import os

TOKEN = os.environ["ARTIFACT_TOKEN"]


class Handler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):  # noqa: N802 - BaseHTTPRequestHandler 的固定方法名
        auth = self.headers.get("Authorization", "")
        if auth != f"Bearer {TOKEN}":
            self.send_response(401)
            self.send_header("WWW-Authenticate", 'Bearer realm="artifacts"')
            self.end_headers()
            self.wfile.write(b"unauthorized\n")
            return
        super().do_GET()

    def log_message(self, fmt, *args):
        print("%s %s" % (self.address_string(), fmt % args), flush=True)


handler = functools.partial(Handler, directory="/srv")
http.server.ThreadingHTTPServer(("", 8080), handler).serve_forever()
