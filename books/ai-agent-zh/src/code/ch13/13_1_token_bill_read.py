# mode: run
# llm: none
# expect: 工具结果
"""读 13_1 的录制文件，把第 5 轮请求拆成四类，按字符数和字数估 token。"""
import json

from lib.budget import text_tokens  # noqa: E402

rows = [json.loads(line) for line in open("ch13/13_1_token_bill.cassette.jsonl", encoding="utf-8") if line.strip()]
last = max(rows, key=lambda r: len(r["request"]["messages"]))
request, usage = last["request"], last["usage"]
prompt_tokens = usage["prompt_tokens"]

groups = {"系统提示": [], "工具定义": [], "对话文本": [], "工具结果": []}
for m in request["messages"]:
    if m["role"] == "system":
        groups["系统提示"].append(m.get("content") or "")
    elif m["role"] == "tool":
        groups["工具结果"].append(m.get("content") or "")
    else:
        text = m.get("content") or ""
        if m.get("tool_calls"):
            text += json.dumps(m["tool_calls"], ensure_ascii=False)
        groups["对话文本"].append(text)
groups["工具定义"].append(json.dumps(request.get("tools") or [], ensure_ascii=False))

chars = {k: sum(len(x) for x in v) for k, v in groups.items()}
total_chars = sum(chars.values())
estimated = {k: text_tokens("".join(v)) for k, v in groups.items()}
fixed = chars["系统提示"] + chars["工具定义"]
total_est = sum(estimated.values())
print(f"第 5 轮 prompt_tokens={prompt_tokens}，请求里 {len(request['messages'])} 条消息")
print(f"{'类别':<6}{'字符':>8}{'字符占比':>10}{'按字数估':>10}{'估算占比':>10}")
for key in groups:
    print(f"{key:<6}{chars[key]:>8}{chars[key] / total_chars:>10.1%}"
          f"{estimated[key]:>10}{estimated[key] / total_est:>10.1%}")
print(f"{'合计':<6}{total_chars:>8}{1:>10.0%}{total_est:>10}{1:>10.0%}")
diff = (sum(estimated.values()) - prompt_tokens) / prompt_tokens
print(f"按字数估的四类合计与 prompt_tokens 相差 {diff:+.1%}")
variable = total_chars - fixed
print(f"去掉系统提示和工具定义，工具结果占可变部分的 {chars['工具结果'] / variable:.0%}")
