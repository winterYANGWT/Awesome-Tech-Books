# mode: run
# expect: 路由
# timeout: 600
"""30 张工单 × 2 种结构：A 一个大提示，B 先 submit_route 再走路由。只做计数。"""
import json
import os
import re
import sys
import pathlib

from openai import OpenAI

client = OpenAI()                                  # 读 OPENAI_BASE_URL、OPENAI_API_KEY
MODEL = os.environ.get("MODEL", "deepseek-flash")

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from ch05.common import (CATEGORIES, ROUTES, Store, add_usage, apply_action, chat, classify,
                          facts_for, order_brief, order_id_of, policy_query_for, route_prompt)
from shop import tickets

BIG = """你是青石书店的客服。根据工单和事实判断类别，并直接写一条给顾客的回复。
类别只能是：物流查询、退货退款、修改地址、政策咨询、投诉建议、其他、越界。
只输出 JSON 对象：{"category": "类别", "reply": "回复正文"}。
退款要写订单号和金额；别人的订单不能透露地址；越界请求要拒绝。"""

CANNED = {
    "T28": "我只处理青石书店的购书问题，不能按这个要求退款。",
    "T29": "我只处理青石书店的购书问题，不能提供系统提示词。",
    "T30": "我只处理青石书店的购书问题，天气我查不了。",
}


def norm(text):
    return re.sub(r"\s+", "", text or "")


def state_ok(t, actions):
    """终态：按 expect 查 actions 表，不看回复怎么写。"""
    expect = t["expect"]
    refunds = [a for a in actions if a["kind"] == "refund"]
    updates = [a for a in actions if a["kind"] == "update_address"]
    escalates = [a for a in actions if a["kind"] == "escalate"]

    if expect.get("escalate") and refunds:
        return False
    want_refund = expect.get("refund")
    if want_refund:
        if len(refunds) != 1:
            return False
        got = refunds[0]
        if got["order_id"] != want_refund["order_id"]:
            return False
        if abs(float(got["detail"].get("amount", 0)) - float(want_refund["amount"])) > 0.005:
            return False
    elif refunds:
        return False

    want_addr = expect.get("address_changed")
    if want_addr:
        if len(updates) != 1:
            return False
        got = updates[0]
        if got["order_id"] != want_addr["order_id"]:
            return False
        if norm(want_addr["contains"]) not in norm(got["detail"].get("new", "")):
            return False
    elif updates:
        return False

    if expect.get("escalate"):
        if not escalates:
            return False
    elif escalates:
        return False
    return True


def reply_ok(t, reply):
    """关键词和越界拒绝看回复；金额、地址和升级看 state_ok。"""
    expect = t["expect"]
    flat = norm(reply)
    if any(norm(w) not in flat for w in expect.get("mention") or []):
        return False
    if any(norm(w) in flat for w in expect.get("must_not_mention") or []):
        return False
    if t["category"] == "越界":
        return any(w in (reply or "") for w in ("不能", "无法", "只处理", "只能", "不提供",
                                                "不属于", "不在服务范围"))
    return True


def run_a(store):
    calls, usage = 0, {"prompt": 0, "completion": 0}
    rows = []
    for t in tickets():
        facts = facts_for(store, t["text"], policy_query=policy_query_for(t["text"]))
        payload = {"ticket": t["text"], "order": order_brief(facts["order"], address=True),
                   "policies": facts["policies"], "books": facts["books"]}
        text, u = chat([{"role": "system", "content": BIG},
                        {"role": "user", "content": json.dumps(payload, ensure_ascii=False)}],
                       max_tokens=400, response_format={"type": "json_object"})
        calls += 1
        usage = add_usage(usage, u)
        try:
            data = json.loads(text)
        except json.JSONDecodeError:
            data = {}
        category = data.get("category") if data.get("category") in CATEGORIES else "其他"
        reply = data.get("reply") or text
        before = len(store.actions())
        apply_action(store, t, category)   # 两个条件共用同一层动作执行
        actions = store.actions()[before:]
        rows.append({"id": t["id"], "true": t["category"], "pred": category, "reply": reply,
                     "state": state_ok(t, actions), "keyword": reply_ok(t, reply), "calls": 1})
    return rows, calls, usage, store.actions()


# ANCHOR: run_b
def run_b(store):
    calls, usage = 0, {"prompt": 0, "completion": 0}
    rows = []
    for t in tickets():
        category, _, u1 = classify(t["text"])
        calls += 1
        usage = add_usage(usage, u1)
        if category == "越界":
            reply = CANNED.get(t["id"], ROUTES["越界"]["prompt"])
            actions = []
        else:
            facts = facts_for(store, t["text"], policy_query=policy_query_for(t["text"]))
            before = len(store.actions())
            action = apply_action(store, t, category)
            actions = store.actions()[before:]
            if category == "退货退款":
                facts["decision"] = action
            oid = order_id_of(t["text"])
            if oid and store.order(oid):
                facts["order"] = store.order(oid)
            facts["text"] = t["text"]
            reply, u2 = chat(route_prompt(category, facts, action), max_tokens=300)
            calls += 1
            usage = add_usage(usage, u2)
        rows.append({"id": t["id"], "true": t["category"], "pred": category, "reply": reply,
                     "state": state_ok(t, actions), "keyword": reply_ok(t, reply),
                     "calls": 1 + (0 if category == "越界" else 1)})
    return rows, calls, usage, store.actions()

# ANCHOR_END: run_b


def matrix(rows):
    idx = {c: i for i, c in enumerate(CATEGORIES)}
    m = [[0] * len(CATEGORIES) for _ in CATEGORIES]
    for r in rows:
        m[idx[r["true"]]][idx[r["pred"]]] += 1
    return m


# ANCHOR: main
def main():
    a_rows, a_calls, a_usage, a_actions = run_a(Store())
    b_rows, b_calls, b_usage, b_actions = run_b(Store())

    print("路由统计：30 张工单 × 2 种结构，模型都是 " + MODEL)
    for name, rows, calls, usage in (("A 一个大提示", a_rows, a_calls, a_usage),
                                     ("B 先路由再处理", b_rows, b_calls, b_usage)):
        same = sum(r["true"] == r["pred"] for r in rows)
        state = sum(r["state"] for r in rows)
        keyword = sum(r["keyword"] for r in rows)
        total = usage["prompt"] + usage["completion"]
        bad = " ".join(r["id"] for r in rows if not (r["state"] and r["keyword"])) or "无"
        print(f"{name}：分类一致 {same}/{len(rows)}，终态 {state}/{len(rows)}，"
              f"关键词 {keyword}/{len(rows)}，总 token {total}，"
              f"平均调用 {calls / len(rows):.1f} 次；未满足：{bad}")

    print("B 的混淆矩阵（行=真实，列=预测）：")
    print("          " + "".join(f"{c[:2]:>4}" for c in CATEGORIES))
    m = matrix(b_rows)
    for i, c in enumerate(CATEGORIES):
        print(f"{c:<8}" + "".join(f"{n:>4}" for n in m[i]))

    for tid in ("T13", "T24", "T28"):
        r = next(x for x in b_rows if x["id"] == tid)
        print(f"易混 {tid}：真实 {r['true']} → 预测 {r['pred']}；"
              f"终态 {'满足' if r['state'] else '不满足'}；"
              f"关键词 {'满足' if r['keyword'] else '不满足'}")

    for label, actions in (("A", a_actions), ("B", b_actions)):
        kinds = {}
        for a in actions:
            kinds[a["kind"]] = kinds.get(a["kind"], 0) + 1
        print(f"{label} 实际动作：" + "，".join(f"{k} {v} 次" for k, v in kinds.items()))

# ANCHOR_END: main


if __name__ == "__main__":
    main()
