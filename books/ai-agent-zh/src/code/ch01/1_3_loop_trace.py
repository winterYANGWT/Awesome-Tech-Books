# mode: run
# llm: none
# expect: 多 2 条
"""读 1_1_agent_loop 的录制文件，看每次请求里 messages 多了什么。

只读 .cassette.jsonl，不调用模型；这是“看底层”的示例。
"""
import json
from pathlib import Path

path = Path("ch01/1_1_agent_loop.cassette.jsonl")
entries = [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines() if line.strip()]

print("录制文件：" + str(path) + f"，共 {len(entries)} 次请求")
prev = 0
for i, e in enumerate(entries, 1):
    msgs = e["request"]["messages"]
    n = len(msgs)
    if i == 1:
        print(f"第 {i} 次请求：messages {n} 条（起始）")
    else:
        print(f"第 {i} 次请求：messages {n} 条（比上次多 {n - prev} 条）")
    new = msgs[prev:] if i > 1 else msgs
    roles = []
    for m in new:
        role = m["role"]
        if m.get("tool_calls"):
            role += "（带 tool_calls）"
        roles.append(role)
    print("  新增：" + "、".join(roles))
    prev = n
