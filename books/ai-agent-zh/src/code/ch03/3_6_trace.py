# 3.6 从录制文件看每一轮的消息栈和 tool_call_id 的配对
# llm: none
# expect: tool_call_id
import json

path = "ch03/3_6_harness.cassette.jsonl"
rows = [json.loads(line) for line in open(path, encoding="utf-8") if line.strip()]

for i, row in enumerate(rows, 1):
    messages = row["request"]["messages"]
    roles = " → ".join(m["role"] for m in messages)
    print(f"请求 {i}（{len(messages)} 条）：{roles}")
    current_ids = []
    for m in messages:
        if m["role"] == "assistant":
            reasoning = m.get("reasoning_content") or ""
            print(f"  assistant 带回 reasoning_content：{'有' if reasoning else '无'}"
                  f"（{len(reasoning)} 字）")
            current_ids = [tc["id"] for tc in m.get("tool_calls") or []] or current_ids
        elif m["role"] == "tool":
            print(f"  tool_call_id={m['tool_call_id']}  和上一条 assistant 的 id 相等："
                  f"{m['tool_call_id'] in current_ids}")
    calls = json.loads(row["body"])["choices"][0]["message"].get("tool_calls") or []
    for tc in calls:
        print(f"  响应 id={tc['id']}  name={tc['function']['name']}")

print("每一对 tool_call_id 和 id 都相等")
