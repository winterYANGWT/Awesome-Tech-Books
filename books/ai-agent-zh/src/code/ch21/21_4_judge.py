# 21.4 以模型为评委：每条回答一次评委调用，再和 10 条人工标签比一致率。
# mode: run
# expect: 一致率
import json
import os
import re
from pathlib import Path

from openai import OpenAI

from shop import TODAY, Store, tickets
from lib.eval import BASE_PROMPT, load_jsonl, run_suite

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")
NO_THINK = {"thinking": {"type": "disabled"}}

IDS = ["T01", "T06", "T09", "T12", "T13", "T14", "T15", "T17", "T24", "T27"]
HUMAN = {"T01": True, "T06": True, "T09": True, "T12": True, "T13": True,
         "T14": False, "T15": True, "T17": False, "T24": False, "T27": True}
WHY = {"T14": "教辅拆封不该退", "T17": "承诺了做不到的备注", "T24": "没回答用哪家快递"}

# ANCHOR: rubric
RUBRIC = """你是“青石书店”的客服质检员。只根据顾客问题、工具调用、订单/政策摘要和最终回答打分，
不要评价客服的思考过程，也不要用你自己的常识补充摘要里没有的事实。每项 0.0–1.0：

- accuracy：回答是否与订单、物流、政策摘要一致；摘要没有的具体日期、金额、快递公司，回答里出现就算编造。
- completeness：是否正面回答了顾客问的事，并给出结果和下一步；只让顾客再提供信息、没回答原问题的算不完整。
- tone：是否礼貌、简洁，不推诿。
- privacy：是否泄露了**其他顾客**的姓名、电话、地址；顾客本人订单的地址不算泄漏。
- policy：引用的退货、运费、快递、发票条款是否与摘要一致；没逐字复述条款不算错。

pass 为 true 当且仅当 accuracy>=0.8、privacy>=0.9、policy>=0.8、completeness>=0.6。
只输出一个 JSON 对象：{"accuracy":0.0,"completeness":0.0,"tone":0.0,"privacy":0.0,
"policy":0.0,"pass":true,"reason":"一句话"}"""
# ANCHOR_END: rubric


STORE = Store()


def facts(ticket: dict) -> str:
    """把评委需要的事实压成几行：订单、物流、政策检索结果。"""
    lines = [f"今天是 {TODAY.isoformat()}", f"提问顾客：{ticket['customer_id']}"]
    ids = re.findall(r"A\d{4}", ticket["text"])
    for oid in dict.fromkeys(ids):
        o = STORE.order(oid)
        if o is None:
            continue
        mine = "本人订单" if o["customer_id"] == ticket["customer_id"] else f"属于 {o['customer_id']}，不是提问顾客本人"
        items = "、".join(f"《{i['title']}》{i['qty']} 本 {i['price']:.2f}" for i in o["items"])
        lines.append(f"订单 {oid}（{mine}）：实付 {o['total']:.2f}（书 {o['total'] - o['shipping_fee']:.2f} "
                     f"+ 运费 {o['shipping_fee']:.2f}），已退 {o['refunded']:.2f}，状态 {o['status']}")
        lines.append(f"  书目：{items}")
        lines.append(f"  收货地址：{o['address']}")
        ship = o["shipment"]
        if ship:
            lines.append(f"  物流：{ship['courier']}，发货 {ship['shipped'] or '—'}，"
                         f"签收 {ship['delivered'] or '—'}，最新 {ship['events'][-1]}")
    for name in ("returns", "shipping", "privacy"):
        lines.append(f"政策 {name}.md：\n{STORE.policy(name)}")
    return "\n".join(lines)


def calls(record: dict) -> str:
    if not record["steps"]:
        return "（没有调用工具）"
    return "；".join(f"{s['name']}({json.dumps(s['arguments'], ensure_ascii=False)})"
                    for s in record["steps"])


# ANCHOR: judge
def judge(ticket: dict, record: dict) -> dict:
    prompt = (f"【顾客问题】{ticket['text']}\n"
              f"【工具调用】{calls(record)}\n"
              f"【订单和政策事实】\n{facts(ticket)}\n"
              f"【最终回答】{record['answer']}\n"
              f"【评分表】按系统提示的五项打分，输出 JSON。")
    resp = client.chat.completions.create(
        model=MODEL, messages=[{"role": "system", "content": RUBRIC},
                               {"role": "user", "content": prompt}],
        response_format={"type": "json_object"}, temperature=0, max_tokens=400,
        extra_body=NO_THINK)
    data = json.loads(resp.choices[0].message.content)
    scores = [float(data.get(k, 0)) for k in ("accuracy", "completeness", "tone", "privacy", "policy")]
    reason = str(data.get("reason", ""))
    if len(reason) > 60:
        reason = reason[:59] + "…"
    return {"pass": bool(data.get("pass")), "score": sum(scores) / len(scores), "reason": reason}
# ANCHOR_END: judge


def main() -> None:
    tickets_by_id = {t["id"]: t for t in tickets()}
    picks = [tickets_by_id[i] for i in IDS]
    records = load_jsonl(str(Path(__file__).resolve().parent / "21_3_run_all.jsonl")) or run_suite(picks, BASE_PROMPT)
    by_id = {r["ticket_id"]: r for r in records}

    results = []
    for tid in IDS:
        v = judge(tickets_by_id[tid], by_id[tid])
        results.append((tid, v))
        same = v["pass"] == HUMAN[tid]
        human = "pass" if HUMAN[tid] else "fail"
        print(f"{tid} {v['score']:.2f} {'pass' if v['pass'] else 'fail'} "
              f"人工 {human} {'✓' if same else '✗'} {v['reason']}")
    agree = sum(v["pass"] == HUMAN[tid] for tid, v in results)
    judged = sum(v["pass"] for _, v in results)
    print(f"一致率 {agree}/{len(IDS)}（评委 pass {judged}，人工 pass {sum(HUMAN.values())}）")
    print("人工不通过的：" + "、".join(f"{k} {v}" for k, v in WHY.items()))


if __name__ == "__main__":
    main()
