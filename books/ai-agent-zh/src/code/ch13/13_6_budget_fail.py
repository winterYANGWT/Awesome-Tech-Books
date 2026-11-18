# mode: fail
# llm: none
# expect: BudgetExceeded
"""调用前估算 8k token，预算 6000：在发请求之前拦住。"""
import json

from lib.budget import categories, count_request
from shop import Store
from shop.tools import SUPPORT_PROMPT, support_tools


class BudgetExceeded(Exception):
    pass


class Budget:
    def __init__(self, max_tokens):
        self.max_tokens = max_tokens

    def check(self, messages, tools):
        cats = categories(messages, tools)
        total = sum(cats.values())
        if total > self.max_tokens:
            top = max(cats, key=cats.get)
            raise BudgetExceeded(f"{total} > {self.max_tokens}，{top}占 {cats[top] / total:.0%}")
        return total


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

cats = categories(messages, TOOLS)
total = count_request(messages, TOOLS)
print(f"当前估算 {total:,}，预算 6000")
for k, v in cats.items():
    print(f"  {k} {v:,} ({v / total:.0%})")
Budget(6000).check(messages, TOOLS)
