# 9.2 同一组工具，ID 换成 UUID 后模型还能不能把上一步的 order_id 用到下一步。
# mode: run
# output: varies
# expect: UUID
# expect: 可读
# timeout: 900
"""T01、T06 各跑 6 次：A 组结果里是固定映射的假 UUID，B 组是可读的 A1659/A1670。"""
import re
import uuid

from harness import ToolError, Toolbox, run, tool

from shop import Store, TODAY, ShopError, tickets

NO_THINK = {"thinking": {"type": "disabled"}}
NS = uuid.UUID("6f1c2a44-9d7e-4b0a-9a5e-2c3d4e5f6071")
TICKETS = {t["id"]: t for t in tickets()}
SYSTEM = f"""你是青石书店的客服。今天是 {TODAY.isoformat()}。
处理步骤：先用 get_order 查订单；物流问题再用 get_shipment 查最新物流；退货退款按订单实付金额调用 issue_refund。
每一步都用上一步结果里的 order_id，不要猜，也不要重复查同一张订单。回答简洁，用中文。"""


# ANCHOR: tools
def fake(kind: str, real: str) -> str:
    """固定映射：同一个订单每次都是同一个 UUID，录制和回放才对得上。"""
    return str(uuid.uuid5(NS, f"shop:{kind}:{real}"))


def make_box(store: Store, scheme: str) -> tuple[Toolbox, dict]:
    to_real: dict[str, str] = {}

    def pub(kind: str, real: str) -> str:
        if scheme == "可读":
            return real
        value = fake(kind, real)
        to_real[value] = real
        return value

    def resolve(value: str) -> str | None:
        # 后端只认自己发出的 ID：UUID 组不接受顾客嘴里的 A1659
        return value if scheme == "可读" else to_real.get(value)

    @tool
    def get_order(order_id: str) -> dict:
        """查一张订单：状态、实付金额、书目、物流。"""
        o = store.order(order_id)
        if o is None:
            raise ToolError(f"订单 {order_id} 不存在，请核对订单号")
        s = o["shipment"]
        return {"order_id": pub("order", o["id"]), "customer_id": pub("customer", o["customer_id"]),
                "book_id": pub("book", o["items"][0]["book_id"]),
                "status": o["status"], "total_paid": o["total"],
                "items": [{"title": i["title"], "qty": i["qty"]} for i in o["items"]],
                "courier": s["courier"], "delivered": s["delivered"],
                "latest": s["events"][-1]}

    @tool
    def get_shipment(order_id: str) -> dict:
        """查一张订单的最新物流状态。"""
        o = store.order(resolve(order_id) or "")
        if o is None:
            raise ToolError(f"订单 {order_id} 不存在，请核对订单号")
        s = o["shipment"]
        return {"order_id": pub("order", o["id"]), "courier": s["courier"],
                "delivered": s["delivered"], "latest": s["events"][-1]}

    @tool(risky=True)
    def issue_refund(order_id: str, amount: float, reason: str) -> dict:
        """给订单退款，钱原路退回。"""
        try:
            real = resolve(order_id)
            if real is None:
                raise ToolError(f"订单 {order_id} 不存在，请核对订单号")
            return store.refund(real, amount, reason)
        except ShopError as e:
            raise ToolError(str(e)) from None

    return Toolbox([get_order, get_shipment, issue_refund]), to_real
# ANCHOR_END: tools


def once(ticket: dict, scheme: str) -> dict:
    store = Store()
    box, to_real = make_box(store, scheme)
    res = run([{"role": "system", "content": SYSTEM}, {"role": "user", "content": ticket["text"]}],
              box, max_turns=5, verbose=False, extra_body=NO_THINK, max_tokens=400)
    want = ticket["expect"]["refund"]
    order_id = want["order_id"] if want else None
    orders = [s for s in res.steps if s.name == "get_order"]
    follow = [s for s in res.steps if s.name in ("get_shipment", "issue_refund")]
    row = {"ok": 0, "fake": 0, "wrong": 0, "stale": 0, "missing": 0, "extra": max(0, len(orders) - 1),
           "done": 0, "prompt": res.usage.get("prompt_tokens", 0)}
    if not order_id:                                  # T01：从工具结果的 order_id 取
        order_id = next((o.arguments.get("order_id") for o in orders), None)
        real_target = order_id
    if not follow:
        row["missing"] = 1
        return row
    got = follow[0].arguments.get("order_id")
    real = to_real.get(got) if scheme == "UUID" else got
    if real == (want["order_id"] if want else real_target):
        row["ok"] = 1
    elif scheme == "UUID" and isinstance(got, str) and re.fullmatch(r"A\d+", got):
        row["stale"] = 1                            # 顾客原话里的旧可读单号
    elif got in to_real:
        row["wrong"] = 1                            # 另一个订单的合法 UUID
    else:
        row["fake"] = 1                             # 编造或对不上的 UUID
    if want:
        acts = [a for a in store.actions() if a["kind"] == "refund"]
        row["done"] = any(abs(a["detail"]["amount"] - want["amount"]) < 0.01 for a in acts)
    else:
        row["done"] = int("迅达快递" in (res.answer or ""))
    return row


# ANCHOR: main
def main() -> None:
    print(f"A1670 的假 UUID：{fake('order', 'A1670')}（固定映射，不是随机数）")
    print("任务  ID     后续调用  ID 正确  错 UUID  旧编号  多查  完成   平均输入 token")
    for tid in ("T01", "T06"):
        for scheme in ("UUID", "可读"):
            rows = [once(TICKETS[tid], scheme) for _ in range(6)]
            n = len(rows)
            print(f"{tid}  {scheme:<4}  {n - sum(r['missing'] for r in rows)}/{n}      "
                  f"{sum(r['ok'] for r in rows)}      "
                  f"{sum(r['fake'] + r['wrong'] for r in rows)}       "
                  f"{sum(r['stale'] for r in rows)}      "
                  f"{sum(r['extra'] for r in rows)}    "
                  f"{sum(r['done'] for r in rows)}/{n}    "
                  f"{sum(r['prompt'] for r in rows) // n}")
    print("说明：两组工具、提示、工单都一样，只换结果里的 ID 写法；错 UUID 指编造或对不上的 UUID，旧编号指把顾客原话里的可读单号原样传回。")


if __name__ == "__main__":
    main()
# ANCHOR_END: main
