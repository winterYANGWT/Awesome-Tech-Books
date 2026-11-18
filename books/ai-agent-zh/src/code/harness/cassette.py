"""读录制文件（`*.cassette.jsonl`），用来展示一次运行里真实发出的请求和收到的响应。

    from harness import cassette
    calls = cassette.load("ch03/3_2_tool_loop.cassette.jsonl")
    print(len(calls), "次模型调用")
    first = calls[0]
    first.request["messages"]      # 发给模型的消息
    first.response                 # 解析后的响应（JSON）
    first.usage                    # token 用量
    print(cassette.brief(first))   # 一行摘要

录制文件由 `scripts/check_examples.py --record` 生成，一行一次调用，按完成的先后排列。
"""
from __future__ import annotations

import json
import pathlib
from dataclasses import dataclass


@dataclass
class Call:
    seq: int
    path: str
    request: dict
    response: dict | str
    status: int
    usage: dict
    latency_ms: int


def load(path: str | pathlib.Path) -> list[Call]:
    calls = []
    for line in pathlib.Path(path).read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        e = json.loads(line)
        body = e["body"]
        if "event-stream" not in e.get("content_type", ""):
            try:
                body = json.loads(body)
            except ValueError:
                pass
        calls.append(Call(e["seq"], e["path"], e["request"], body, e["status"], e.get("usage", {}),
                          e.get("latency_ms", 0)))
    return calls


def brief(call: Call) -> str:
    """一行摘要：第几次调用、发了几条消息、带了几个工具、模型回了什么、用了多少 token。"""
    req = call.request
    msgs = req.get("messages", [])
    roles = "/".join(m.get("role", "?")[0] for m in msgs)
    out = "?"
    if isinstance(call.response, dict) and call.response.get("choices"):
        m = call.response["choices"][0]["message"]
        if m.get("tool_calls"):
            out = "要工具 " + ", ".join(tc["function"]["name"] for tc in m["tool_calls"])
        else:
            text = " ".join((m.get("content") or "").split())
            out = "回答 " + (text[:20] + "…" if len(text) > 20 else text)
    u = call.usage
    return (f"#{call.seq + 1} 消息 {len(msgs)} 条（{roles}） 工具 {len(req.get('tools', []))} 个 → {out}"
            f"  [{u.get('prompt_tokens', '?')}+{u.get('completion_tokens', '?')} token]")
