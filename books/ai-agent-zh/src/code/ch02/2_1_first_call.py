"""2.1 一次只有两条消息的调用：没有工具，模型看不到订单，只能猜金额。"""
# mode: run
# expect: prompt_tokens=\d+
# expect: 元
import os

from openai import OpenAI

from shop import Store, tickets

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")

SYSTEM = "你是青石书店的在线客服。回答用中文，简洁，直接告诉顾客结果和下一步。"

ticket = next(t for t in tickets() if t["id"] == "T06")
messages = [
    {"role": "system", "content": SYSTEM},
    {"role": "user", "content": ticket["text"] + "\n能退我多少钱？你估个大概的数就行，不用精确。"},
]

response = client.chat.completions.create(
    model=MODEL,
    messages=messages,
    temperature=0.7,
    max_tokens=1200,
)
choice = response.choices[0]
usage = response.usage

print(f"finish_reason={choice.finish_reason}")
print(f"prompt_tokens={usage.prompt_tokens} completion_tokens={usage.completion_tokens} "
      f"total_tokens={usage.total_tokens}")
print("回复前 120 字：" + " ".join((choice.message.content or "").split())[:120])

# 程序另外查一次库，用来对照；这条数据没有发给模型。
print(f"实际可退（程序查库，模型没看到）：{Store().order('A1659')['total']:.2f} 元")
