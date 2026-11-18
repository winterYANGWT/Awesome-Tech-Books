# mode: run
# timeout: 300
# expect: 82.00
"""12 条消息的 T06 会话：先按预算清旧工具结果，再交给 harness 回答并退款。"""
import json
import os

from openai import OpenAI

from lib.budget import count_request, trim_to_budget
from harness import run
from shop import Store, tickets
from shop.tools import SUPPORT_PROMPT, support_tools

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")
store = Store()
box = support_tools(store)
TOOLS = box.schemas()
T06 = next(t for t in tickets() if t["id"] == "T06")

# 一段旧的工单查询结果：真实场景里是几十条历史记录，占了大半个窗口。
old = store.query("SELECT id, created, status FROM orders ORDER BY id LIMIT 100")
LOG = json.dumps([{"order_id": o["id"], "date": o["created"], "status": o["status"],
                   "note": f"旧工单：顾客询问退货，已按当时的期限答复；处理人客服{i % 7 + 1}，"
                           f"回访{'完成' if i % 2 else '未完成'}。"} for i, o in enumerate(old)],
                 ensure_ascii=False)


def call(cid, name, args):
    return {"role": "assistant", "content": "", "tool_calls": [
        {"id": cid, "type": "function", "function": {"name": name, "arguments": json.dumps(args, ensure_ascii=False)}}]}


messages = [
    {"role": "system", "content": SUPPORT_PROMPT},
    {"role": "user", "content": "先把上次那批退货工单调出来，我要核对 A1469 那单。"},
    call("call_old", "get_order_log", {}),
    {"role": "tool", "tool_call_id": "call_old", "content": LOG},
    {"role": "assistant", "content": "旧工单摘要：" + "；".join(
        f"{o['id']}（{o['created']}，{o['status']}）顾客问退货，客服按当时条款答复并回访"
        for o in old[:55]) + "。其中 A1469 退过 48 元，已经处理完。"},
    {"role": "user", "content": T06["text"]},
    call("call_a1659", "get_order", {"order_id": "A1659"}),
    {"role": "tool", "tool_call_id": "call_a1659", "content": box.call("get_order", {"order_id": "A1659"})},
    {"role": "assistant", "content": "A1659 实付 82 元，书有缺页，属于质量问题，可以退全款。"},
    {"role": "user", "content": "按缺页退全款 82 元，别动 A1469。"},
    call("call_policy", "search_policies", {"query": "质量问题 退货"}),
    {"role": "tool", "tool_call_id": "call_policy", "content": box.call("search_policies", {"query": "质量问题 退货"})},
]

before = count_request(messages, TOOLS)
trimmed = trim_to_budget(messages, 6000, TOOLS)
after = count_request(trimmed, TOOLS)
print(f"before={before:,} after={after:,} 消息={len(messages)}→{len(trimmed)} 预算=6000")

res = run(messages, box, model=MODEL, max_turns=6, verbose=False,
          before_model=lambda ms: trim_to_budget(ms, 6000, TOOLS),
          extra_body={"thinking": {"type": "disabled"}})
print(f"实际累计 prompt={res.usage.get('prompt_tokens', 0):,}（{res.turns} 轮）")
for a in store.actions():
    if a["kind"] == "refund":
        print(f"issue_refund {a['order_id']} {a['detail']['amount']:.2f}")
print("回答：" + " ".join((res.answer or "").split())[:60])
