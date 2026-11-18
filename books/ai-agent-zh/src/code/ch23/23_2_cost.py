# mode: run
# llm: none
# expect: 合计
# expect: 最贵的一轮
"""读 23.1 的录制文件里的 usage，按轮聚合 token，再用 prices.json 的占位单价算“计费单位”。

计费单位 = 未命中输入 / 1000 × 输入单价 + 命中输入 / 1000 × 缓存单价 + 输出 / 1000 × 输出单价。
单价只是占位值，所以只看每轮之间的比例，不要把单位当成钱。
"""
import json
import pathlib

CASSETTE = pathlib.Path("ch23/23_1_trace.cassette.jsonl")
PRICES = json.loads(pathlib.Path("ch23/prices.json").read_text(encoding="utf-8"))
PER = PRICES["per_tokens"]


def units(hit: int, miss: int, out: int) -> float:
    return (miss * PRICES["input_miss"] + hit * PRICES["input_hit"] + out * PRICES["output"]) / PER


def main():
    calls = [json.loads(line) for line in CASSETTE.read_text(encoding="utf-8").splitlines() if line.strip()]
    rows = []
    for call in calls:
        req = call["request"]
        try:
            usage = json.loads(call["body"])["usage"]
        except ValueError:                     # 流式响应没有整段 body，退回代理记下的 usage
            usage = call["usage"]
        hit = usage.get("prompt_cache_hit_tokens", 0)
        miss = usage.get("prompt_cache_miss_tokens", usage["prompt_tokens"] - hit)
        reasoning = (usage.get("completion_tokens_details") or {}).get("reasoning_tokens", 0)
        rows.append({"messages": len(req.get("messages", [])), "prompt": usage["prompt_tokens"],
                     "hit": hit, "miss": miss, "out": usage["completion_tokens"],
                     "reasoning": reasoning, "units": units(hit, miss, usage["completion_tokens"])})

    print(f"单价（占位，每 {PER:,} token）：未命中 {PRICES['input_miss']}，命中 {PRICES['input_hit']}，输出 {PRICES['output']}")
    print(" 轮  消息   prompt  cache_hit  cache_miss  completion  计费单位")
    for i, r in enumerate(rows, 1):
        print(f"{i:>2}  {r['messages']:>3}  {r['prompt']:>7,}  {r['hit']:>9,}  {r['miss']:>10,}  "
              f"{r['out']:>10,}  {r['units']:>9.4f}")

    prompt = sum(r["prompt"] for r in rows)
    hit = sum(r["hit"] for r in rows)
    miss = sum(r["miss"] for r in rows)
    out = sum(r["out"] for r in rows)
    total = sum(r["units"] for r in rows)
    input_units = (miss * PRICES["input_miss"] + hit * PRICES["input_hit"]) / PER
    output_units = out * PRICES["output"] / PER
    print(f"合计  {prompt:>7,}  {hit:>9,}  {miss:>10,}  {out:>10,}  {total:>9.4f}")
    print(f"缓存命中率 {hit / prompt:.1%}；输入 {input_units:.4f} 单位（{input_units / total:.1%}），"
          f"输出 {output_units:.4f} 单位（{output_units / total:.1%}）")

    worst = max(range(len(rows)), key=lambda i: rows[i]["units"])
    print(f"最贵的一轮：第 {worst + 1} 轮，{rows[worst]['units']:.4f} 单位，占 {rows[worst]['units'] / total:.0%}")
    print(f"缓存少花 {hit * (PRICES['input_miss'] - PRICES['input_hit']) / PER:.4f} 单位（这些输入如果全按未命中算）")

    seen, tools = set(), {}
    for call in calls:
        names = {}
        for msg in call["request"]["messages"]:
            for tc in msg.get("tool_calls") or []:
                names[tc["id"]] = tc["function"]["name"]
            if msg.get("role") == "tool" and msg.get("tool_call_id") in names:
                tid = msg["tool_call_id"]
                if tid not in seen:
                    seen.add(tid)
                    tools[names[tid]] = tools.get(names[tid], 0) + len(msg["content"].encode())
    print("工具结果：" + "，".join(f"{name} {size} B" for name, size in tools.items())
          + "；这些字节在后面的每一轮里都会重发")


if __name__ == "__main__":
    main()
