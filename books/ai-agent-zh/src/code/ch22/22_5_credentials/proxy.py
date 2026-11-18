"""MCP 代理：沙箱只带 session id 来，代理拿 vault 里的 token 去调外部服务。"""
from __future__ import annotations

import json
import threading
import urllib.request
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

from vault import Vault


class Handler(BaseHTTPRequestHandler):
    def _send(self, status: int, body: dict) -> None:
        data = json.dumps(body, ensure_ascii=False, sort_keys=True).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    # ANCHOR: proxy
    def do_GET(self) -> None:  # noqa: N802
        if self.path == "/service":
            token = self.server.vault.get("mcp:orders")
            if self.headers.get("Authorization") != f"Bearer {token}":
                self._send(401, {"error": "缺少或错误的 bearer token"})
                return
            self._send(200, {"ok": True, "tool": "get_order", "order_id": "A1659", "total_paid": 82.0})
        elif self.path == "/tool":
            session = self.headers.get("X-Session", "")
            if session not in self.server.sessions:
                self._send(403, {"error": "未知的 session"})
                return
            token = self.server.vault.get(self.server.sessions[session])
            req = urllib.request.Request(self.server.base + "/service",
                                         headers={"Authorization": f"Bearer {token}"})
            try:
                with urllib.request.urlopen(req, timeout=3) as resp:
                    self._send(resp.status, json.loads(resp.read()))
            except Exception as exc:  # noqa: BLE001
                self._send(502, {"error": type(exc).__name__})
        else:
            self._send(404, {"error": "not found"})
    # ANCHOR_END: proxy

    def log_message(self, *args) -> None:  # 不打印访问日志，输出只留给示例
        pass


class ProxyServer:
    def __init__(self, vault: Vault):
        self.vault = vault
        self.sessions = {"S-T06": "mcp:orders"}

    def __enter__(self) -> "ProxyServer":
        self.httpd = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        self.httpd.vault, self.httpd.sessions = self.vault, self.sessions
        self.base = f"http://127.0.0.1:{self.httpd.server_port}"
        self.httpd.base = self.base
        self.thread = threading.Thread(target=self.httpd.serve_forever, daemon=True)
        self.thread.start()
        return self

    def __exit__(self, *exc) -> None:
        self.httpd.shutdown()
        self.httpd.server_close()
        self.thread.join(timeout=2)
