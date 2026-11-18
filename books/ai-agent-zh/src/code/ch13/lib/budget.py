"""本章示例共用的 token 估算、按预算裁剪和工具消息配对检查。

不是精确的 tokenizer：按汉字和 ASCII 字符分别估，再用实际 `usage.prompt_tokens` 校准。
"""
from __future__ import annotations

import copy
import json


class ToolPairError(Exception):
    """assistant 的 tool_calls 和 role: tool 没有成对。"""


def _is_cjk(ch: str) -> bool:
    return "\u3400" <= ch <= "\u9fff" or "\u3000" <= ch <= "\u303f" or "\uff00" <= ch <= "\uffef"


def text_tokens(text: str) -> int:
    """粗略估算：一个汉字约 1 token，4 个 ASCII 字符约 1 token。"""
    text = str(text)
    cjk = sum(1 for ch in text if _is_cjk(ch))
    return int(cjk + (len(text) - cjk) * 0.25 + 0.999)


def message_tokens(m: dict) -> int:
    n = text_tokens(m.get("content") or "")
    if m.get("tool_calls"):
        n += text_tokens(json.dumps(m["tool_calls"], ensure_ascii=False))
    return n


def tools_tokens(tools: list | None) -> int:
    return text_tokens(json.dumps(tools or [], ensure_ascii=False))


def categories(messages: list, tools: list | None = None) -> dict[str, int]:
    """系统提示、工具定义、对话文本、工具结果四类的估算 token。"""
    out = {"系统提示": 0, "工具定义": tools_tokens(tools), "对话": 0, "工具结果": 0}
    for m in messages:
        if m["role"] == "system":
            out["系统提示"] += message_tokens(m)
        elif m["role"] == "tool":
            out["工具结果"] += message_tokens(m)
        else:
            out["对话"] += message_tokens(m)
    return out


def count_request(messages: list, tools: list | None = None) -> int:
    return sum(categories(messages, tools).values())


def check_pairs(messages: list) -> None:
    """每个 tool_calls[i].id 都要有一条同值 tool_call_id 的 tool 消息，顺序也要对。"""
    pending: list[str] = []
    for m in messages:
        if m["role"] == "assistant" and m.get("tool_calls"):
            pending = [tc["id"] for tc in m["tool_calls"]]
        elif m["role"] == "tool":
            if not pending:
                raise ToolPairError(f"tool 消息的 tool_call_id={m.get('tool_call_id')} 前面没有对应的 tool_calls")
            if m.get("tool_call_id") not in pending:
                raise ToolPairError(f"tool 消息的 tool_call_id={m.get('tool_call_id')} 不在上一轮 tool_calls 里")
            pending.remove(m["tool_call_id"])
        elif pending:
            raise ToolPairError(f"tool_calls 里的 {pending[0]} 没有对应的 tool_call_id 消息")


def stub(content: str) -> str:
    """清掉旧工具结果的正文，只留订单号、金额、状态等关键事实。"""
    try:
        data = json.loads(content)
    except (json.JSONDecodeError, TypeError):
        return "[已清理] " + " ".join(str(content).split())[:40]
    if isinstance(data, dict) and "order_id" in data:
        amount = data.get("total_paid", data.get("amount", data.get("refunded_total")))
        return f"[已清理] 订单 {data['order_id']}，金额 {amount}，状态 {data.get('status', '—')}"
    if isinstance(data, list) and data and isinstance(data[0], dict) and "section" in data[0]:
        return "[已清理] 政策条款：" + "；".join(str(x["section"]) for x in data[:3])
    if isinstance(data, list) and data and isinstance(data[0], dict) and "order_id" in data[0]:
        return f"[已清理] {len(data)} 张旧订单，从 {data[0]['order_id']} 到 {data[-1]['order_id']}"
    if isinstance(data, dict) and "recent_orders" in data:
        return f"[已清理] 顾客 {data.get('customer_id')}，最近订单 {len(data['recent_orders'])} 张"
    return "[已清理] 旧工具结果"


def trim_to_budget(messages: list, max_tokens: int, tools: list | None = None, keep_last: int = 4) -> list:
    """保护系统提示 → 保留最近 keep_last 条 → 旧工具结果换 stub → 仍超预算再成对删中间轮。"""
    msgs = copy.deepcopy(messages)
    n_sys = 1 if msgs and msgs[0]["role"] == "system" else 0
    tail = max(n_sys, len(msgs) - keep_last)
    for i in range(n_sys, tail):
        if msgs[i]["role"] == "tool":
            msgs[i]["content"] = stub(msgs[i]["content"])
    while count_request(msgs, tools) > max_tokens:
        units, i = [], n_sys
        while i < tail:
            j = i + 1
            if msgs[i]["role"] == "assistant" and msgs[i].get("tool_calls"):
                while j < len(msgs) and msgs[j]["role"] == "tool":
                    j += 1
            units.append((i, j))
            i = j
        if not units:
            break
        start, end = units[0]
        del msgs[start:end]
        tail -= end - start
        if tail <= n_sys:
            break
    return msgs
