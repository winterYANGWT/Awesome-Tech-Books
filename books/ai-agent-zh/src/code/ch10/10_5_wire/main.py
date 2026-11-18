"""起一个 stdio server，依次发 discover、tools/list、tools/call，打印原始行。"""
# llm: none
# expect: server/discover
# expect: tools/call
# expect: resultType

import json
import pathlib
import subprocess
import sys

SERVER = pathlib.Path(__file__).with_name("server.py")


def meta():
    return {
        "io.modelcontextprotocol/protocolVersion": "2026-07-28",
        "io.modelcontextprotocol/clientInfo": {"name": "ch10-wire-client", "version": "0.1.0"},
        "io.modelcontextprotocol/clientCapabilities": {},
    }


def send(proc, message):
    line = json.dumps(message, ensure_ascii=False, separators=(",", ":"))
    print("→ " + line)
    proc.stdin.write(line + "\n")
    proc.stdin.flush()
    reply_line = proc.stdout.readline().strip()
    print("← " + reply_line)
    reply = json.loads(reply_line)
    assert reply["id"] == message["id"], f"id 对不上：{reply['id']} != {message['id']}"
    return reply


with subprocess.Popen([sys.executable, str(SERVER)], stdin=subprocess.PIPE,
                      stdout=subprocess.PIPE, text=True, encoding="utf-8") as proc:
    discover = send(proc, {"jsonrpc": "2.0", "id": 1, "method": "server/discover",
                           "params": {"_meta": meta()}})
    listing = send(proc, {"jsonrpc": "2.0", "id": 2, "method": "tools/list",
                          "params": {"_meta": meta()}})
    call = send(proc, {"jsonrpc": "2.0", "id": 3, "method": "tools/call",
                       "params": {"name": "add", "arguments": {"a": 2, "b": 3}, "_meta": meta()}})

print()
print(f"三个响应的 id：{discover['id']}、{listing['id']}、{call['id']}；"
      f"结果都带 resultType={call['result']['resultType']}；第一条是 server/discover，没有 initialize。")
