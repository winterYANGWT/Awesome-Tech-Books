"""2.6 输出上限：只给 32 个 token，却要求写 100 字。"""
# mode: run
# expect: finish_reason=length
# expect: 以句号结尾：False
import os

from openai import OpenAI

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")

response = client.chat.completions.create(
    model=MODEL,
    messages=[
        {"role": "system", "content": "你是青石书店的客服。只输出正文，不要标题和列表。"},
        {"role": "user", "content": "写一段 100 字左右的退款说明，告诉顾客缺页属于质量问题，可以全额退款。"},
    ],
    max_tokens=32,
    extra_body={"thinking": {"type": "disabled"}},
)
choice, usage = response.choices[0], response.usage
content = choice.message.content or ""

print(f"finish_reason={choice.finish_reason}")
print(f"completion_tokens={usage.completion_tokens}")
print(f"content={content}")
print(f"以句号结尾：{content.rstrip().endswith('。')}")
