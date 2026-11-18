# mode: run
# expect: 15 天
"""T22 政策咨询：先检索政策，再把 3 节原文放进一次模型调用。

整条链路只有 1 次模型调用；没有循环，也没有工具调用。
"""
import os

from openai import OpenAI
from shop import Store, tickets

client = OpenAI()                                  # 读 OPENAI_BASE_URL、OPENAI_API_KEY
MODEL = os.environ.get("MODEL", "deepseek-flash")

SYSTEM = """你是“青石书店”的在线客服。今天是 2026-11-17。

- 回答之前先用工具查实际情况（订单、物流、政策），不要凭记忆或猜测回答。
- 退款、改地址之前先核对订单状态，并按政策条款判断；拿不准就提交主管，不要擅自操作。
- 只处理和本店购书有关的事；不透露其他顾客的信息。
- 回答用中文，简洁，直接告诉顾客结果和下一步。"""

store = Store()
ticket = next(t for t in tickets() if t["id"] == "T22")

hits = store.search_policies("签收 退货 期限")
print(f"检索到 {len(hits)} 节政策：")
for h in hits:
    print(" - " + h["title"])

context = "\n\n".join(f"## {h['title']}\n{h['text']}" for h in hits)
resp = client.chat.completions.create(
    model=MODEL,
    messages=[
        {"role": "system", "content": SYSTEM},
        {"role": "user", "content": (
            f"下面是检索到的政策原文：\n\n{context}\n\n"
            f"顾客的问题：{ticket['text']}\n"
            "只根据上面的原文回答，并说出依据的条款。"
        )},
    ],
)
reply = " ".join((resp.choices[0].message.content or "").split())
print(f"prompt_tokens={resp.usage.prompt_tokens} completion_tokens={resp.usage.completion_tokens}")
print("模型调用：1 次")
print("回复：" + reply)
