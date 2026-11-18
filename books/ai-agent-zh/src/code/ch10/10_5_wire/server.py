#!/usr/bin/env python3
"""玩具 MCP server：从 stdin 一行读一条 JSON-RPC，把响应写到 stdout。不依赖 MCP SDK。"""
import json
import sys

PROTOCOL = "2026-07-28"
SERVER_INFO = {"name": "ch10-wire-demo", "version": "0.1.0"}
TOOLS = [{
    "name": "add",
    "title": "加法",
    "description": "两个整数相加",
    "inputSchema": {
        "type": "object",
        "properties": {"a": {"type": "integer"}, "b": {"type": "integer"}},
        "required": ["a", "b"],
    },
    "annotations": {"readOnlyHint": True, "openWorldHint": False},
}]
REQUIRED_META = ["io.modelcontextprotocol/protocolVersion",
                 "io.modelcontextprotocol/clientCapabilities"]


def ok(mid, payload):
    return {"jsonrpc": "2.0", "id": mid,
            "result": {"resultType": "complete", **payload,
                       "_meta": {"io.modelcontextprotocol/serverInfo": SERVER_INFO}}}


def fail(mid, code, message):
    return {"jsonrpc": "2.0", "id": mid, "error": {"code": code, "message": message}}


def handle(msg):
    mid = msg.get("id")
    meta = msg.get("params", {}).get("_meta", {})
    missing = [key for key in REQUIRED_META if key not in meta]
    if missing:
        return fail(mid, -32602, f"Missing required _meta field: {', '.join(missing)}")

    method = msg.get("method")
    if method == "server/discover":
        return ok(mid, {"supportedVersions": [PROTOCOL], "capabilities": {"tools": {}},
                        "instructions": "玩具 server：只有一个 add 工具", "ttlMs": 0,
                        "cacheScope": "private"})
    if method == "tools/list":
        return ok(mid, {"tools": TOOLS, "ttlMs": 0, "cacheScope": "private"})
    # ANCHOR: call
    if method == "tools/call":
        params = msg.get("params", {})
        if params.get("name") != "add":
            return fail(mid, -32602, f"Unknown tool: {params.get('name')}")
        args = params.get("arguments", {})
        try:
            value = int(args["a"]) + int(args["b"])
        except (KeyError, TypeError, ValueError):
            return ok(mid, {"content": [{"type": "text", "text": "参数必须是两个整数 a、b"}],
                            "isError": True})
        return ok(mid, {"content": [{"type": "text", "text": str(value)}],
                        "structuredContent": {"sum": value}})
    # ANCHOR_END: call
    return fail(mid, -32601, f"Method not found: {method}")


for line in sys.stdin:
    line = line.strip()
    if not line:
        continue
    try:
        request = json.loads(line)
    except json.JSONDecodeError as exc:
        reply = {"jsonrpc": "2.0", "id": None,
                 "error": {"code": -32700, "message": f"Parse error: {exc.msg}"}}
    else:
        reply = handle(request)
    print(json.dumps(reply, ensure_ascii=False, separators=(",", ":")), flush=True)
