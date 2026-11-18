# mode: run
# expect: 闸门
"""T06 的闸门：第一版故意漏订单号，被拒后把错误回填，第二版通过。"""
import json
import os
import re
import sys
import pathlib

from openai import OpenAI

client = OpenAI()                                  # 读 OPENAI_BASE_URL、OPENAI_API_KEY
MODEL = os.environ.get("MODEL", "deepseek-flash")

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from ch05.common import Store, draft_reply, facts_for, gate_draft, known_summary
from shop import tickets

ticket = next(t for t in tickets() if t["id"] == "T06")
store = Store()
facts = facts_for(store, ticket["text"], policy_query="退货 质量问题 退款 运费")
facts["text"] = ticket["text"]
summary = known_summary(facts)                    # 5.1 已经演示过摘要，这里直接用同样的契约
order = facts["order"]
print("[摘要] " + json.dumps(summary, ensure_ascii=False))

# 第一版：模型照常写，但这里故意把订单号抹掉，固定住“漏字段”这个现象。
draft1, u1 = draft_reply(summary, facts)
draft1 = re.sub(r"A\d{4}\s*订单", "您的订单", draft1)
draft1 = re.sub(r"A\d{4}", "您的订单", draft1)
print("[草稿 v1] " + draft1.replace("\n", " "))
gate_v1 = gate_draft(draft1, summary, order)
print("[闸门 v1] " + json.dumps(gate_v1, ensure_ascii=False))

# 把“哪条不合格、允许什么”写回生成步，重写一次。
assert not gate_v1["pass"], "第一版应当被闸门拦下"
draft2, u2 = draft_reply(summary, facts, gate_v1["issues"], extra="这一版第一句必须写：订单 A1659。")
print("[草稿 v2] " + draft2.replace("\n", " "))
gate_v2 = gate_draft(draft2, summary, order)
print("[闸门 v2] " + json.dumps(gate_v2, ensure_ascii=False))
print(f"[用量] 两次草稿 {u1['prompt']}+{u1['completion']}、{u2['prompt']}+{u2['completion']} token")
print(f"[结论] {'第二版通过，可以发出' if gate_v2['pass'] else '两次都不过，转人工'}")
