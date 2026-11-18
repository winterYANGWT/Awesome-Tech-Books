# mode: run
# expect: 第 5 轮
"""T06 的五轮对话：每轮把前缀原样发一次，读 usage.prompt_tokens。

工具调用和结果按固定脚本放在对话里，每轮只换顾客的下一句话；
这样五轮的账单形状可复现，不会因为模型这次想多查一次就变长。
"""
import json
import os

from openai import OpenAI

from shop import Store
from shop.tools import SUPPORT_PROMPT, support_tools

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")
store = Store()
box = support_tools(store)
TOOLS = box.schemas()

FOLLOW_UPS = [
    "这算质量问题吗？政策上是怎么规定的？",
    "那这单就按缺页退全款吧。对了，我上次 A1469 那单退了多少？",
    "退款多久能到账？",
    "我上次那单退了 48 元，这次别退重复了。",
]
PLAN = [
    [("get_order", {"order_id": "A1659"}), ("search_policies", {"query": "质量问题 退货"}),
     ("list_reviews", {"book_id": "B12"})],
    [("get_customer_context", {"customer_id": "C082"}), ("get_order", {"order_id": "A1469"}),
     ("search_policies", {"query": "退款 到账 时间"}), ("list_reviews", {"book_id": "B12"})],
    [("get_order", {"order_id": "A1659"}), ("search_policies", {"query": "签收后多久能退货"}),
     ("get_order", {"order_id": "A1469"})],
    [("get_customer_context", {"customer_id": "C082"}), ("get_order", {"order_id": "A1659"}),
     ("search_policies", {"query": "退款 金额 权限"})],
]


def add_tools(messages, calls):
    """把一组工具调用和真实结果追加到对话；id 固定，便于阅读录制文件。"""
    turn = len(messages)
    ids = [f"call_fixed_{turn}_{i}" for i in range(len(calls))]
    messages.append({"role": "assistant", "content": "", "tool_calls": [
        {"id": cid, "type": "function", "function": {"name": name, "arguments": json.dumps(args, ensure_ascii=False)}}
        for cid, (name, args) in zip(ids, calls)]})
    for cid, (name, args) in zip(ids, calls):
        messages.append({"role": "tool", "tool_call_id": cid, "content": box.call(name, args)})


messages = [{"role": "system", "content": SUPPORT_PROMPT},
            {"role": "user", "content": "收到的《大运河两千年》中间缺了二十多页，订单号 A1659，怎么处理？"}]

for turn in range(1, 6):
    resp = client.chat.completions.create(model=MODEL, messages=messages, tools=TOOLS,
                                          max_tokens=500, extra_body={"thinking": {"type": "disabled"}})
    u = resp.usage
    n_tool = sum(1 for m in messages if m["role"] == "tool")
    print(f"第 {turn} 轮 prompt={u.prompt_tokens} completion={u.completion_tokens} "
          f"工具结果={n_tool} 缓存命中={u.prompt_cache_hit_tokens}")
    if turn == 5:
        break
    messages.append({"role": "assistant", "content": resp.choices[0].message.content or "好的，我先查一下。"})
    messages.append({"role": "user", "content": FOLLOW_UPS[turn - 1]})
    add_tools(messages, PLAN[turn - 1])
