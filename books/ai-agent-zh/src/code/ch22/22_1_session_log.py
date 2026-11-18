# 22.1 把一次客服对话写成 append-only 的 JSONL，再从头重放成消息列表。
# mode: run
# llm: none
# expect: seq=8
# expect: role: system → user → assistant → tool → assistant → tool → assistant
"""事件日志只追加；消息列表是它的一个投影，随时可以由事件重建。"""
from __future__ import annotations

import json
import pathlib

LOG = pathlib.Path("ch22/session.jsonl")


def event(seq: int, kind: str, payload: dict) -> dict:
    return {"seq": seq, "type": kind, "payload": payload,
            "created_at": f"2026-11-17T09:00:{seq:02d}+08:00"}


EVENTS = [
    event(1, "session_started", {"session_id": "S-T06", "system": "你是青石书店的客服。今天是 2026-11-17。"}),
    event(2, "user", {"content": "A1659 的《大运河两千年》缺了二十多页，怎么处理？"}),
    event(3, "assistant", {"content": "", "reasoning_content": "先查 A1659 的订单，确认实付金额和签收状态。",
                           "tool_calls": [{
        "id": "call_1", "type": "function",
        "function": {"name": "get_order", "arguments": json.dumps({"order_id": "A1659"})}}]}),
    event(4, "tool_result", {"tool_call_id": "call_1",
                             "content": '{"order_id":"A1659","total_paid":82.0,"status":"已签收"}'}),
    event(5, "assistant", {"content": "", "reasoning_content": "订单已签收、实付 82 元，按缺页做全额退款。",
                           "tool_calls": [{
        "id": "call_2", "type": "function",
        "function": {"name": "issue_refund", "arguments": json.dumps(
            {"order_id": "A1659", "amount": 82.0, "reason": "缺页"})}}]}),
    event(6, "tool_result", {"tool_call_id": "call_2",
                             "content": '{"order_id":"A1659","amount":82.0,"status":"已退款"}'}),
    event(7, "answer", {"content": "已为订单 A1659 退款 82 元，退回原支付方式。"}),
    event(8, "session_ended", {"reason": "answer"}),
]


# ANCHOR: replay
def replay(events: list[dict]) -> list[dict]:
    """事件 → 下一次模型调用要看的 messages：只保留能重建消息的类型。"""
    messages: list[dict] = []
    for e in events:
        p, kind = e["payload"], e["type"]
        if kind == "session_started":
            messages.append({"role": "system", "content": p["system"]})
        elif kind == "user":
            messages.append({"role": "user", "content": p["content"]})
        elif kind == "assistant":
            m = {"role": "assistant", "content": p["content"]}
            if p.get("reasoning_content"):
                m["reasoning_content"] = p["reasoning_content"]
            if p.get("tool_calls"):
                m["tool_calls"] = p["tool_calls"]
            messages.append(m)
        elif kind == "tool_result":
            messages.append({"role": "tool", "tool_call_id": p["tool_call_id"], "content": p["content"]})
        elif kind == "answer":
            m = {"role": "assistant", "content": p["content"]}
            if p.get("reasoning_content"):
                m["reasoning_content"] = p["reasoning_content"]
            messages.append(m)
        # tool_call（执行意图）和 session_ended 不进入 messages
    return messages
# ANCHOR_END: replay


def main() -> None:
    LOG.parent.mkdir(parents=True, exist_ok=True)
    with LOG.open("w", encoding="utf-8") as f:
        for e in EVENTS:
            f.write(json.dumps(e, ensure_ascii=False) + "\n")
    print(f"写入 {LOG}：{len(EVENTS)} 个事件")
    for e in EVENTS:
        print(f"seq={e['seq']} type={e['type']}")

    loaded = [json.loads(line) for line in LOG.read_text(encoding="utf-8").splitlines()]
    messages = replay(loaded)
    print(f"\n重放：{len(messages)} 条消息")
    print("role: " + " → ".join(m["role"] for m in messages))
    calls = [tc["function"]["name"] for m in messages for tc in m.get("tool_calls", [])]
    tool_ids = {m["tool_call_id"] for m in messages if m["role"] == "tool"}
    print(f"tool_calls: {', '.join(calls)}；tool_call_id 对得上：{len(tool_ids) == len(calls)}")


if __name__ == "__main__":
    main()
