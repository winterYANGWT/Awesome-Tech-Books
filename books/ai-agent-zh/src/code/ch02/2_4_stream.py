"""2.4 流式输出：打印前几个非空片段，最后拼成完整回答。"""
# mode: run
# expect: 拼接后
import os

from openai import OpenAI

from shop import tickets

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")
SYSTEM = "你是青石书店的在线客服。回答用中文，简洁，直接告诉顾客结果和下一步。"

ticket = next(t for t in tickets() if t["id"] == "T06")
messages = [
    {"role": "system", "content": SYSTEM},
    {"role": "user", "content": ticket["text"] +
     "\n用三句话回复顾客：先道歉，说明这是质量问题可以全额退款，再告诉他下一步。不要用列表。"},
]

stream = client.chat.completions.create(
    model=MODEL,
    messages=messages,
    max_tokens=400,
    stream=True,
    stream_options={"include_usage": True},
    extra_body={"thinking": {"type": "disabled"}},
)

pieces, chunks, last = [], 0, None
for chunk in stream:
    chunks += 1
    last = chunk
    if not chunk.choices:
        continue
    piece = chunk.choices[0].delta.content
    if piece:
        pieces.append(piece)
        if len(pieces) <= 5:
            print(f"片段 {len(pieces)}：{piece}")

print(f"chunk 总数：{chunks}")
print("拼接后：" + "".join(pieces))
if last is not None and last.choices and last.usage:
    usage = last.usage
    print(f"最后一个 chunk：finish_reason={last.choices[0].finish_reason} "
          f"usage={usage.prompt_tokens}+{usage.completion_tokens}={usage.total_tokens}")
