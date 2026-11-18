# mode: run
# timeout: 300
# expect: 82.00
"""同一段 8k 消息：先清旧工具结果，再成对裁剪，降到 6k 预算内并完成 T06。"""
import json
import os

from openai import OpenAI

from lib.budget import count_request, trim_to_budget
from harness import run
from shop import Store
from shop.tools import SUPPORT_PROMPT, support_tools

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")
store = Store()
box = support_tools(store)
TOOLS = box.schemas()


def call(cid, name, args):
    return {"role": "assistant", "content": "", "tool_calls": [
        {"id": cid, "type": "function", "function": {"name": name, "arguments": json.dumps(args, ensure_ascii=False)}}]}


old = store.query("SELECT id, created, status, total FROM orders ORDER BY id LIMIT 92")
LOG = json.dumps([{"order_id": o["id"], "date": o["created"], "status": o["status"],
                   "note": "旧工单：顾客询问退货，已按当时期限答复；回访完成。"} for o in old], ensure_ascii=False)
SUMMARY = "旧工单摘要：" + "；".join(
    f"{o['id']}（{o['created']}，{o['status']}，{o['total']:.2f} 元）顾客询问退货，客服按当时条款答复并回访"
    for o in old[:65]) + "。其中 A1469 退过 48 元，已经处理完；其余没有未结事项。"

messages = [
    {"role": "system", "content": SUPPORT_PROMPT},
    {"role": "user", "content": "先把上次那批退货工单调出来，我要核对 A1469 那单，然后处理 A1659 的缺页。"},
    call("c1", "get_order_log", {}),
    {"role": "tool", "tool_call_id": "c1", "content": LOG},
    {"role": "assistant", "content": SUMMARY},
    {"role": "user", "content": "收到的《大运河两千年》中间缺了二十多页，订单号 A1659，怎么处理？"},
    call("c2", "get_order", {"order_id": "A1659"}),
    {"role": "tool", "tool_call_id": "c2", "content": box.call("get_order", {"order_id": "A1659"})},
    {"role": "assistant", "content": "A1659 实付 82 元，书有缺页。按退货政策的质量问题条款，缺页属于质量问题，"
                                    "退全款，不受 15 天限制，签收后 60 天内都可以申请；100 元以下可以直接退。"},
    {"role": "user", "content": "按缺页退全款 82 元，别动 A1469。"},
    call("c3", "search_policies", {"query": "质量问题 退货"}),
    {"role": "tool", "tool_call_id": "c3", "content": box.call("search_policies", {"query": "质量问题 退货"})},
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
