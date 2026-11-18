# 16.4 同一批 6 个案例：A 只给规程全文让模型心算，B 给 Skill 并强制调用 refund.py。
# mode: run
# expect: 6/6
# timeout: 300
import json
import os
import re
import subprocess
import sys
from pathlib import Path

from openai import OpenAI

from ch16.lib.skillkit import load_skill, tokens

client = OpenAI()                                  # 读 OPENAI_BASE_URL、OPENAI_API_KEY
MODEL = os.environ.get("MODEL", "deepseek-flash")
HERE = Path(__file__).resolve().parent
SKILL_DIR = HERE / "16_3_refund_skill" / "refund-handling"
SCRIPT = SKILL_DIR / "scripts" / "refund.py"

# ANCHOR: cases
CASES = [
    ("C1", "A1618", "quality", False, 118.80, "订单 A1618 打 9 折，实付 118.80 元；书在运输中破损，要求退款。应退多少？"),
    ("C2", "A1660", "changed_mind", False, 50.00, "订单 A1660 实付 56 元，其中运费 6 元；《北方的河》买重复了，想退这一本。应退多少？"),
    ("C3", "A1667", "changed_mind", False, 52.25, "订单 A1667 实付 58.25 元，其中运费 6 元；不想要了，在 15 天内退货。应退多少？"),
    ("C4", "A1680", "quality", False, 680.00, "订单 A1680 实付 680 元；典藏版三卷装订脱落，要求全额退款。应退多少？"),
    ("C5", "A1649", "changed_mind", True, 0.00, "订单 A1649 是两本教辅，实付 64 元；已经拆开做了两页，不想要了。应退多少？"),
    ("C6", "A1659", "quality", False, 82.00, "订单 A1659 实付 82 元，其中运费 6 元；书中间缺了二十多页。应退多少？"),
]
# ANCHOR_END: cases


def run_script(order: str, reason: str, opened: bool) -> tuple[float, dict]:
    cmd = [sys.executable, str(SCRIPT), "--order", order, "--reason", reason]
    if opened:
        cmd.append("--opened")
    cp = subprocess.run(cmd, cwd=SKILL_DIR, capture_output=True, text=True, timeout=30)
    data = json.loads(cp.stdout)
    return data["amount"], data


def parse_amount(text: str) -> float | None:
    m = re.search(r"AMOUNT:\s*([0-9]+(?:\.[0-9]+)?)", text or "")
    return round(float(m.group(1)), 2) if m else None


def main() -> None:
    skill, _ = load_skill(SKILL_DIR)
    assert skill is not None
    procedure = skill.body + "\n\n" + (SKILL_DIR / "references" / "returns.md").read_text(encoding="utf-8")
    print(f"A 条件：规程全文 {tokens(procedure)} token，模型心算（思考关闭）")
    print(f"B 条件：Skill 正文 {skill.body_tokens} token + refund.py（思考关闭，只统计一次工具调用）")
    print()

    a_hits, a_tokens = 0, 0
    print("A：只给规程全文")
    for cid, order, reason, opened, want, question in CASES:
        prompt = f"按下面的退款规程计算，只输出最后一行 AMOUNT: <两位小数>。\n\n{procedure}\n\n案例：{question}"
        resp = client.chat.completions.create(model=MODEL, messages=[{"role": "user", "content": prompt}],
                                              extra_body={"thinking": {"type": "disabled"}})
        text = resp.choices[0].message.content or ""
        got = parse_amount(text)
        a_tokens += resp.usage.prompt_tokens + resp.usage.completion_tokens
        ok = got is not None and abs(got - want) < 0.005
        a_hits += ok
        print(f"  {cid} 应退 {want:.2f}，模型 {got if got is not None else '?'} {'✓' if ok else '✗'}")
    print(f"  A 精确命中 {a_hits}/6，平均 {a_tokens // 6} token\n")

    b_hits, b_tokens = 0, 0
    print("B：Skill + refund.py（强制调用；金额来自脚本 stdout，没有回给模型）")
    tool_schema = {"type": "function", "function": {
        "name": "compute_refund",
        "description": "运行 refund.py 计算应退金额。reason 用 quality 或 changed_mind；教辅拆封时 opened=true。",
        "parameters": {"type": "object", "properties": {
            "order": {"type": "string"}, "reason": {"type": "string", "enum": ["quality", "changed_mind"]},
            "opened": {"type": "boolean"}}, "required": ["order", "reason"]}}}
    for cid, order, reason, opened, want, question in CASES:
        messages = [{"role": "system", "content": skill.body}, {"role": "user", "content": question}]
        resp = client.chat.completions.create(
            model=MODEL, messages=messages, tools=[tool_schema],
            tool_choice={"type": "function", "function": {"name": "compute_refund"}},
            extra_body={"thinking": {"type": "disabled"}})
        b_tokens += resp.usage.prompt_tokens + resp.usage.completion_tokens
        call = resp.choices[0].message.tool_calls[0]
        args = json.loads(call.function.arguments or "{}")
        got, data = run_script(args["order"], args["reason"], bool(args.get("opened")))
        ok = abs(got - want) < 0.005
        b_hits += ok
        print(f"  {cid} 应退 {want:.2f}，脚本 {got:.2f} {'✓' if ok else '✗'}")
    print(f"  B 精确命中 {b_hits}/6，平均 {b_tokens // 6} token")
    print(f"\n结论：脚本 {b_hits}/6，心算 {a_hits}/6；B {b_tokens // 6} token/案例，"
          f"A {a_tokens // 6} token/案例（{b_tokens // 6 - a_tokens // 6:+d}）")


if __name__ == "__main__":
    main()
