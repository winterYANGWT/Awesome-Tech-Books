# 9.6 get_order v1（10 个字段的 JSON）和 v2（9 个字段的 Markdown）在 8 张工单上的对比。
# mode: run
# output: varies
# expect: v1
# expect: v2
# timeout: 1200
"""8 张工单 × 2 版 × 4 次 = 64 次运行：完成率、输入 token、轮数、错误数。"""
from harness import ToolError, Toolbox, run, tool

from shop import Store, tickets
from shop.tools import SUPPORT_PROMPT, support_tools

NO_THINK = {"thinking": {"type": "disabled"}}
SYSTEM = SUPPORT_PROMPT + "\n- 顾客的请求符合政策时，直接调用工具办理，不要只在回复里描述；缺少必要信息时才追问。"
IDS = ["T01", "T06", "T07", "T09", "T11", "T15", "T17", "T22"]
TICKETS = {t["id"]: t for t in tickets()}
REPEAT = 4


# ANCHOR: box
def make_box(store: Store, version: str) -> Toolbox:
    box = support_tools(store, include=("get_order", "search_policies", "issue_refund",
                                        "update_address", "escalate_to_supervisor"))
    if version == "v1":
        return box

    @tool
    def get_order(order_id: str) -> str:
        """查一张订单：状态、金额、书目、物流。返回 Markdown 文本。"""
        o = store.order(order_id)
        if o is None:
            raise ToolError(f"订单 {order_id} 不存在，请核对订单号（形如 A1234）")
        s = o["shipment"]
        lines = [f"订单 {o['id']}（顾客 {o['customer_id']}）",
                 f"状态：{o['status']}", f"下单：{o['created']}",
                 f"实付：{o['total']:.2f} 元；已退：{o['refunded']:.2f} 元；运费：{o['shipping_fee']:.2f} 元",
                 "商品："]
        for i in o["items"]:
            paid = round(i["price"] * i["qty"] * o["discount"], 2)
            lines.append(f"- {i['book_id']} {i['title']} ×{i['qty']} 实付 {paid:.2f} 元")
        if s:
            lines += [f"物流：{s['courier']}；签收：{s['delivered'] or '未签收'}",
                      f"最新：{s['events'][-1]}"]
        else:
            lines.append("物流：未发货")
        return "\n".join(lines)

    box.tools["get_order"] = get_order
    return box
# ANCHOR_END: box


def judge(store: Store, ticket: dict, answer: str, stop: str) -> tuple[bool, bool]:
    """返回（完成，退款金额正确）。完成按工单的 expect 查终态或答案里的必提词。"""
    exp = ticket["expect"]
    if exp["refund"]:
        want = exp["refund"]
        acts = [a for a in store.actions() if a["kind"] == "refund"]
        hit = [a for a in acts if a["order_id"] == want["order_id"]
               and abs(a["detail"]["amount"] - want["amount"]) < 0.01]
        return bool(hit), bool(hit)
    if exp["address_changed"]:
        want = exp["address_changed"]
        hit = [a for a in store.actions() if a["kind"] == "update_address"
               and a["order_id"] == want["order_id"] and want["contains"] in a["detail"]["new"]]
        return bool(hit), True
    said = exp["mention"]
    if ticket["id"] == "T17":                       # “已发货”“已经发货”都算说到了状态
        ok = stop == "answer" and any(m in answer for m in ("已发货", "已经发货", "发货了"))
    else:
        ok = stop == "answer" and all(m in answer for m in said)
    bad = exp.get("must_not_mention") or []
    ok = ok and not any(m in answer for m in bad)
    return ok, True


def once(ticket: dict, version: str) -> dict:
    store = Store()
    res = run([{"role": "system", "content": SYSTEM},
               {"role": "user", "content": ticket["text"]}],
              make_box(store, version), max_turns=6, verbose=False,
              extra_body=NO_THINK, max_tokens=600)
    done, amount_ok = judge(store, ticket, res.answer or "", res.stop)
    errs = sum(s.result.startswith("错误：") for s in res.steps) + (res.stop == "max_turns")
    return {"done": done, "amount": amount_ok, "prompt": res.usage.get("prompt_tokens", 0),
            "turns": res.turns, "errors": errs}


# ANCHOR: main
def main() -> None:
    rows = {v: {tid: [once(TICKETS[tid], v) for _ in range(REPEAT)] for tid in IDS}
            for v in ("v1", "v2")}
    print("工单  v1 完成  v2 完成   退款金额（v1/v2）")
    for tid in IDS:
        r1, r2 = rows["v1"][tid], rows["v2"][tid]
        amt = ""
        if TICKETS[tid]["expect"]["refund"]:
            amt = f"{sum(r['amount'] for r in r1)}/{REPEAT}  {sum(r['amount'] for r in r2)}/{REPEAT}"
        print(f"{tid}   {sum(r['done'] for r in r1)}/{REPEAT}      "
              f"{sum(r['done'] for r in r2)}/{REPEAT}       {amt}")
    for v in ("v1", "v2"):
        allr = [r for tid in IDS for r in rows[v][tid]]
        n = len(allr)
        print(f"{v} 汇总：完成 {sum(r['done'] for r in allr)}/{n}，"
              f"平均输入 token {sum(r['prompt'] for r in allr) / n:.0f}，"
              f"平均轮数 {sum(r['turns'] for r in allr) / n:.1f}，"
              f"工具错误 {sum(r['errors'] for r in allr)}")
    o = Store().order("A1659")
    paid = round(o["items"][0]["price"] * o["discount"], 2)
    print(f"字段检查：v2 保留运费 {o['shipping_fee']:.2f}；T06 应退 {o['total']:.2f} = "
          f"书款 {paid:.2f} + 运费 {o['shipping_fee']:.2f}（裁掉运费就只能算 {paid:.2f}）")


if __name__ == "__main__":
    main()
# ANCHOR_END: main
