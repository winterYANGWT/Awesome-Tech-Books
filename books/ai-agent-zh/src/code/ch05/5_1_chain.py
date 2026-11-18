# mode: run
# expect: 82
"""T06 的四步链：取事实 → 摘要 → 草稿 → 闸门。只有摘要和草稿调模型。"""
import json
import os
import sys
import pathlib

from openai import OpenAI

client = OpenAI()                                  # 读 OPENAI_BASE_URL、OPENAI_API_KEY
MODEL = os.environ.get("MODEL", "deepseek-flash")

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from ch05.common import (TODAY, Store, add_usage, draft_reply, facts_for, gate_draft,
                          order_brief, summarize)
from shop import tickets

ticket = next(t for t in tickets() if t["id"] == "T06")
store = Store()

# 第 1 步：取事实。两个本地查询，不调模型。
facts = facts_for(store, ticket["text"], policy_query="退货 质量问题 退款 运费")
facts["text"] = ticket["text"]
order = facts["order"]
print(f"[1 取事实] {order['id']} 实付 {order['total']:.1f} 元，已退 {order['refunded']:.1f} 元")
print("           政策：" + "；".join(p["section"] for p in facts["policies"]))

# 第 2 步：摘要。输出契约是固定字段的 JSON。
summary, sum_usage, raw_summary = summarize(facts)
print("[2 摘要] " + json.dumps(summary, ensure_ascii=False))
print(f"           token {sum_usage['prompt']}+{sum_usage['completion']}")

# 第 3 步：草稿。输入只有摘要和顾客原话，不带前两步的整段历史。
draft, draft_usage, issues = "", {"prompt": 0, "completion": 0}, []
gate = {"pass": False, "issues": []}
for attempt in (1, 2):
    draft, u = draft_reply(summary, facts, issues)
    draft_usage = add_usage(draft_usage, u)
    gate = gate_draft(draft, summary, order)
    print(f"[3 草稿 v{attempt}] " + draft.replace("\n", " "))
    print(f"              token {u['prompt']}+{u['completion']}；闸门 pass={gate['pass']}")
    if gate["pass"]:
        break
    if any(not i["retryable"] for i in gate["issues"]):
        print("[3 草稿] 有不可重试的问题，停止重写")
        break
    issues = gate["issues"]

# 第 4 步：闸门。它是代码，不是模型调用。
print("[4 闸门] " + json.dumps(gate, ensure_ascii=False))

if not gate["pass"]:
    raise SystemExit("需人工处理：草稿没过闸门")

rows = [("取事实", "—", 0, 0, f"{order['id']} / {summary.get('amount')} / {len(facts['policies'])} 条政策"),
        ("摘要", MODEL, sum_usage["prompt"], sum_usage["completion"], f"amount={summary.get('amount')}"),
        ("草稿", MODEL, draft_usage["prompt"], draft_usage["completion"], f"{order['id']} / {summary.get('amount')} 元"),
        ("闸门", "—", 0, 0, f"pass={gate['pass']}")]
print("步骤      模型            输入   输出  关键字段")
for r in rows:
    print(f"{r[0]:<8}  {r[1]:<14}  {r[2]:>4}  {r[3]:>4}  {r[4]}")
print(f"{'合计':<8}  {'—':<14}  {sum_usage['prompt'] + draft_usage['prompt']:>4}  "
      f"{sum_usage['completion'] + draft_usage['completion']:>4}  今天是 {TODAY}")
