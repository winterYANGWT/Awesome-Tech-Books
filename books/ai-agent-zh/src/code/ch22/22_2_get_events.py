# 22.2 同一个 session，按 token 预算取三种切片：全量、退款前 3 个、最近 6 个。
# mode: run
# llm: none
# expect: 最近 6 个
"""事件一直在日志里；给模型看多少，是 harness 每次现算的。"""
from __future__ import annotations

import json
import math

CORPUS = "订单状态已核对物流记录签收信息完整政策条款适用退款金额计算无误顾客诉求清楚。"
FILLER = (CORPUS * 200)


def filler(n: int) -> str:
    return FILLER[:max(0, n)]


def estimate(text: str) -> int:
    """粗估：一个汉字 1 token，四个 ASCII 字符 1 token。只用来比较切片大小。"""
    cjk = sum(1 for ch in text if "\u4e00" <= ch <= "\u9fff")
    ascii_chars = sum(1 for ch in text if ord(ch) < 128)
    return cjk + math.ceil(ascii_chars / 4)


def message_of(e: dict) -> dict | None:
    p, kind = e["payload"], e["type"]
    if kind == "session_started":
        return {"role": "system", "content": p["system"]}
    if kind == "user":
        return {"role": "user", "content": p["content"]}
    if kind == "assistant":
        m = {"role": "assistant", "content": p["content"]}
        if p.get("reasoning_content"):
            m["reasoning_content"] = p["reasoning_content"]
        if p.get("tool_calls"):
            m["tool_calls"] = p["tool_calls"]
        return m
    if kind == "tool_result":
        return {"role": "tool", "tool_call_id": p["tool_call_id"], "content": p["content"]}
    if kind == "decision":
        return {"role": "system", "content": "已有的决定：" + p["note"]}
    if kind == "answer":
        m = {"role": "assistant", "content": p["content"]}
        if p.get("reasoning_content"):
            m["reasoning_content"] = p["reasoning_content"]
        return m
    return None


def tokens(events: list[dict]) -> int:
    return sum(estimate(json.dumps(m, ensure_ascii=False)) for m in map(message_of, events) if m)


def messages(events: list[dict]) -> int:
    return sum(1 for e in events if message_of(e))


def event(seq: int, kind: str, target: int, **payload) -> dict:
    """造一个事件，并让它的消息部分大约占 target 个 token。"""
    if kind == "session_started":
        payload["system"] = ""
    elif kind == "decision":
        payload["note"] = ""
    else:
        payload.setdefault("content", "")
    e = {"seq": seq, "type": kind, "payload": payload, "created_at": f"2026-11-17T10:00:{seq:02d}+08:00"}
    base = tokens([e])
    if kind == "session_started":
        e["payload"]["system"] = filler(target)
    elif kind == "decision":
        e["payload"]["note"] = filler(target - base)
    else:
        e["payload"]["content"] = filler(target - base)
    return e


def make_events() -> list[dict]:
    out = [event(1, "session_started", 40, session_id="S-T06"),
           event(2, "user", 80, content_hint="T06")]
    names = ["get_order", "search_policies", "get_customer_context", "list_reviews", "get_order",
             "search_policies", "get_order", "search_books", "get_order", "search_policies",
             "get_customer_context", "get_order", "search_policies", "get_order"]
    for i in range(14):
        seq = 3 + i * 2
        target = 295 if seq < 29 else 200
        out.append(event(seq, "assistant", target, content="", reasoning_content="继续核对订单和物流。",
                         tool_calls=[{
            "id": f"call_{seq}", "type": "function",
            "function": {"name": names[i], "arguments": json.dumps({"order_id": "A1659"})}}]))
        out.append(event(seq + 1, "tool_result", target, tool_call_id=f"call_{seq}"))
    out.append(event(31, "decision", 200, action="refund", order_id="A1659", amount=82))
    out.append(event(32, "tool_call", 0, tool_call_id="call_31", name="issue_refund",
                     arguments={"order_id": "A1659", "amount": 82.0}, idempotency_key="S-T06:call_31"))
    out.append(event(33, "tool_result", 250, tool_call_id="call_31"))
    out.append(event(34, "assistant", 170, content="", reasoning_content="等顾客回复。"))
    out.append(event(35, "user", 245, content=""))
    out.append(event(36, "assistant", 245, content="", reasoning_content="再核对一次订单。",
                     tool_calls=[{
        "id": "call_36", "type": "function",
        "function": {"name": "get_order", "arguments": json.dumps({"order_id": "A1659"})}}]))
    out.append(event(37, "tool_result", 245, tool_call_id="call_36"))
    out.append(event(38, "assistant", 245, content="", reasoning_content="整理退款说明。"))
    out.append(event(39, "answer", 245, content="", reasoning_content="把金额和原因写清楚。"))
    out.append(event(40, "session_ended", 0, reason="answer"))
    return out


class SessionLog:
    def __init__(self, events: list[dict]):
        self.events = events

    # ANCHOR: get_events
    def get_events(self, after: int | None = None, before: int | None = None, n: int | None = None):
        rows = self.events
        if after is not None:
            rows = [e for e in rows if e["seq"] > after]
        if before is not None:
            rows = [e for e in rows if e["seq"] < before][-n:] if n else [e for e in rows if e["seq"] < before]
        elif n:
            rows = rows[-n:]
        return rows

    # ANCHOR_END: get_events

    def last(self, n: int):
        return self.events[-n:]


def show(label: str, events: list[dict]) -> None:
    print(f"{label}：{tokens(events):,} token，{messages(events)} 条消息（{len(events)} 个事件）")


def main() -> None:
    log = SessionLog(make_events())
    refund = next(e["seq"] for e in log.events if e["type"] == "tool_call")
    show("全量 40 个事件", log.events)
    show(f"after={refund}（退款之后继续）", log.get_events(after=refund))
    show(f"退款事件 seq={refund} 前 3 个（29–31）", log.get_events(before=refund, n=3))
    show("最近 6 个（35–40）", log.last(6))
    print("\n同一条退款事件，三种取法；日志本身一条也没删。")


if __name__ == "__main__":
    main()
