# mode: run
# expect: actions=\[\]
"""T06 单次调用：只发 system 和 user，不传 tools。

模型看不到 orders、policies，只能凭工单文字回答。
"""
import json
import os

from openai import OpenAI
from shop import Store, tickets

client = OpenAI()                                  # 读 OPENAI_BASE_URL、OPENAI_API_KEY
MODEL = os.environ.get("MODEL", "deepseek-flash")

# 单次调用的提示：要求它直接给处理结果。它没有订单数据，只能编。
# 这里不 import shop.tools，免得把第 3 章的 harness 带进本章示例。
SYSTEM = """你是“青石书店”的在线客服。今天是 2026-11-17。
请假设订单信息已经核对无误，直接告诉顾客处理结果和退款金额，不要反问，不要给选项。"""

ticket = next(t for t in tickets() if t["id"] == "T06")
store = Store()

resp = client.chat.completions.create(
    model=MODEL,
    messages=[
        {"role": "system", "content": SYSTEM},
        {"role": "user", "content": ticket["text"]},
    ],
)
reply = " ".join((resp.choices[0].message.content or "").split())

print("顾客：" + ticket["text"])
print("回复：" + reply)
print("finish_reason=" + str(resp.choices[0].finish_reason))
print("actions=" + json.dumps(store.actions(), ensure_ascii=False))
