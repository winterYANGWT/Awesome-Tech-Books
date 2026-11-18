# mode: fail
# llm: none
# expect: tool_call_id
"""朴素地保留最近 6 条：从一条 tool 回复开始，前面的 assistant tool_calls 被切掉。"""
import json

from lib.budget import check_pairs
from shop import Store
from shop.tools import SUPPORT_PROMPT, support_tools

store = Store()
box = support_tools(store)
T06 = "收到的《大运河两千年》中间缺了二十多页，订单号 A1659，怎么处理？"


def call(cid, name, args):
    return {"role": "assistant", "content": "", "tool_calls": [
        {"id": cid, "type": "function", "function": {"name": name, "arguments": json.dumps(args, ensure_ascii=False)}}]}


messages = [
    {"role": "system", "content": SUPPORT_PROMPT},
    {"role": "user", "content": "先把上次那批退货工单调出来。"},
    call("call_old", "get_order_log", {}),
    {"role": "tool", "tool_call_id": "call_old", "content": "[{\"order_id\": \"A1469\"}]"},
    {"role": "user", "content": T06},
    call("call_a1659", "get_order", {"order_id": "A1659"}),
    {"role": "tool", "tool_call_id": "call_a1659", "content": box.call("get_order", {"order_id": "A1659"})},
    {"role": "assistant", "content": "A1659 实付 82 元，缺页，可以退全款。"},
    {"role": "user", "content": "按缺页退全款 82 元。"},
    call("call_policy", "search_policies", {"query": "质量问题 退货"}),
    {"role": "tool", "tool_call_id": "call_policy", "content": box.call("search_policies", {"query": "质量问题 退货"})},
    call("call_refund", "issue_refund", {"order_id": "A1659", "amount": 82, "reason": "缺页"}),
]

trimmed = messages[-6:]
print(f"原始 {len(messages)} 条，保留最近 {len(trimmed)} 条：{[m['role'] for m in trimmed]}")
check_pairs(trimmed)
print("校验通过")
