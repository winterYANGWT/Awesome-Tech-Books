# mode: run
# expect: T13.*主管
# timeout: 600
"""三张工单走完整流水线：先路由，再在路由内部跑链；T13 在生成前就分流。"""
import json
import os
import sys
import pathlib

from openai import OpenAI

client = OpenAI()                                  # 读 OPENAI_BASE_URL、OPENAI_API_KEY
MODEL = os.environ.get("MODEL", "deepseek-flash")

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from ch05.common import (ROUTES, Store, add_usage, chat, classify, draft_reply, facts_for,
                          gate_draft, order_brief, route_prompt, summarize, TODAY)
from shop import tickets


def log(step, model, usage, key):
    return {"step": step, "model": model, "in": usage.get("prompt", 0),
            "out": usage.get("completion", 0), "key": key}


def run(ticket, store):
    rows = []
    category, _, u = classify(ticket["text"])
    rows.append(log("route", "deepseek-flash", u, category))
    facts = facts_for(store, ticket["text"], policy_query="退货 质量问题 退款 运费")
    facts["text"] = ticket["text"]
    order = facts["order"]
    route_model = ROUTES[category]["model"]

    if category == "退货退款" and order:
        # 生成前分流：金额超限的工单不进草稿链。
        if order["total"] - order["refunded"] > 500:
            store.escalate(order["id"], f"{order['total']:g} 元退款需主管批准")
            return category, rows, f"{order['id']} 已转主管（{order['total']:g} 元超限，未进入草稿）"
        summary, u, _ = summarize(facts, model=route_model)
        rows.append(log("summary", route_model, u, f"amount={summary.get('amount')}"))
        issues, gate = [], {"pass": False, "issues": []}
        for attempt in (1, 2):
            draft, u = draft_reply(summary, facts, issues, model=route_model)
            rows.append(log(f"draft{attempt}", route_model, u, order["id"]))
            gate = gate_draft(draft, summary, order)
            if gate["pass"]:
                break
            if any(not i["retryable"] for i in gate["issues"]):
                store.escalate(order["id"], "草稿有不可重试的问题")
                return category, rows, f"{order['id']} 有不可重试的问题，已转人工"
            issues = gate["issues"]
        if not gate["pass"]:
            store.escalate(order["id"], "草稿两次没过闸门")
            return category, rows, f"{order['id']} 草稿两次没过闸门，已转人工"
        amount = summary.get("amount")
        if amount and amount <= order["total"] - order["refunded"]:
            store.refund(order["id"], amount, "质量问题" if any(w in ticket["text"] for w in ("缺页", "缺了", "装订", "破损")) else "顾客退货")
        return category, rows, draft

    # 政策咨询和其余路由：一次按该路由提示生成
    reply, u = chat(route_prompt(category, facts), model=route_model)
    rows.append(log("reply", route_model, u, category))
    return category, rows, reply


store = Store()
for tid in ("T06", "T22", "T13"):
    t = next(x for x in tickets() if x["id"] == tid)
    category, rows, reply = run(t, store)
    print(f"{tid} 路由 → {category}；回复：{reply.replace(chr(10), ' ')}")
    print("      步骤：" + "；".join(f"{r['step']} {r['model']} {r['in']}+{r['out']} [{r['key']}]" for r in rows))

print("今天：" + TODAY.isoformat())
print("实际动作：" + json.dumps([{"kind": a["kind"], "order": a["order_id"], "detail": a["detail"]}
                                for a in store.actions()], ensure_ascii=False))
