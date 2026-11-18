"""只要求 Markdown 代码块里的 JSON：客户端却直接 json.loads。"""
# mode: fail
# expect: JSONDecodeError
import json
import os

from openai import OpenAI

from shop import tickets

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")

ticket = next(t for t in tickets() if t["id"] == "T06")
SYSTEM = """你是青石书店的客服助理。把工单分类，输出一个 json 对象，字段是 category 和 order_id。
必须用 Markdown 的 ```json 代码块把 json 包起来：第一行是 ```json，最后一行是 ```。
代码块外不要写任何字。"""
resp = client.chat.completions.create(
    model=MODEL,
    messages=[{"role": "system", "content": SYSTEM}, {"role": "user", "content": ticket["text"]}],
    max_tokens=600,
)
content = resp.choices[0].message.content or ""
print(f"finish_reason={resp.choices[0].finish_reason}")
print(f"content 开头：{content[:50]!r}")
obj = json.loads(content)
print(obj)
