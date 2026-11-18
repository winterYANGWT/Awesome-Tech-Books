"""自由文本 + 更严格的正则：第一条就卡住。"""
# mode: fail
# expect: 无法解析
import os
import re

from openai import OpenAI

from shop import tickets

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")

ticket = next(t for t in tickets() if t["id"] == "T06")
resp = client.chat.completions.create(
    model=MODEL,
    messages=[{"role": "system", "content": "你是青石书店的客服助理。用一句话说明这张工单的类别。"},
              {"role": "user", "content": ticket["text"]}],
    max_tokens=600,
)
raw = resp.choices[0].message.content or ""
print(f"原文：{' '.join(raw.split())}")

m = re.search(r"CATEGORY:\s*(\S+)", raw)
if not m:
    raise ValueError("无法解析模型输出；解析器期望的格式是 CATEGORY: 退货退款")
print(f"解析结果：{m.group(1)}")
