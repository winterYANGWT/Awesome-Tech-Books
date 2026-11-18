"""2.3 同一个问题：默认思考一次，关掉思考一次。"""
# mode: run
# expect: reasoning_tokens
import os

from openai import OpenAI

from shop import tickets

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")
SYSTEM = "你是青石书店的在线客服。回答用中文，简洁，直接告诉顾客结果和下一步。"

ticket = next(t for t in tickets() if t["id"] == "T06")
messages = [
    {"role": "system", "content": SYSTEM},
    {"role": "user", "content": ticket["text"] + "\n能退我多少钱？你估个大概的数就行，不用精确。"},
]

for label, extra in [("默认", None), ("关闭", {"thinking": {"type": "disabled"}})]:
    kwargs = {"extra_body": extra} if extra else {}
    response = client.chat.completions.create(model=MODEL, messages=messages, max_tokens=1200, **kwargs)
    message, usage = response.choices[0].message, response.usage
    reasoning = " ".join((getattr(message, "reasoning_content", None) or "").split())
    details = usage.completion_tokens_details
    reasoning_tokens = getattr(details, "reasoning_tokens", 0) or 0
    print(f"{label}：reasoning_content={reasoning[:60] or '（空）'}")
    print(f"  reasoning_tokens={reasoning_tokens} completion_tokens={usage.completion_tokens}")
    print("  回答：" + " ".join((message.content or "").split())[:80])
