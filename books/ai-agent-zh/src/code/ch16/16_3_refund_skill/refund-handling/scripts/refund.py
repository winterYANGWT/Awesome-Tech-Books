#!/usr/bin/env python3
"""算一张订单应退多少钱。只读，不修改订单。

    python scripts/refund.py --order A1659 --reason quality
    python scripts/refund.py --order A1649 --reason changed_mind --opened

stdout 是一行 JSON：{"order_id": ..., "amount": ..., "basis": ..., "needs_approval": ..., "refundable": ...}
stderr 是诊断。退出码：0 算出结果（包括不可退）；2 参数不对；3 订单不存在。
"""
from __future__ import annotations

import argparse
import datetime as dt
import json
import pathlib
import sys

# 让脚本从任意目录运行都能 import shop：src/code/ch16/<示例>/<skill>/scripts/
CODE = pathlib.Path(__file__).resolve().parents[4]
sys.path.insert(0, str(CODE))

from shop import TODAY, Store  # noqa: E402

LIMIT = 500.0


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description="计算订单应退金额；只读，不退款。",
        epilog="退出码：0 算出结果（refundable 可能为 false）；2 参数不对；3 订单不存在。",
    )
    p.add_argument("--order", required=True, help="订单号，例如 A1659")
    p.add_argument("--reason", required=True, choices=["quality", "changed_mind"],
                   help="quality=质量问题；changed_mind=不想要/买错/重复购买")
    p.add_argument("--book", help="只退这一本（书目编号，例如 B30）；不填表示整单")
    p.add_argument("--opened", action="store_true", help="教辅已经拆封；非质量问题时不退款")
    return p.parse_args()


def money(x: float) -> float:
    return round(float(x) + 1e-9, 2)


def main() -> int:
    args = parse_args()
    store = Store()
    order = store.order(args.order)
    if order is None:
        print(f"订单 {args.order} 不存在；订单号形如 A1659", file=sys.stderr)
        return 3

    delivered = order["shipment"]["delivered"] if order["shipment"] else None
    days = (TODAY - dt.date.fromisoformat(delivered)).days if delivered else None
    print(f"[refund] order={order['id']} reason={args.reason} delivered={delivered} days={days}",
          file=sys.stderr)

    items = order["items"]
    selected = [i for i in items if args.book is None or i["book_id"] == args.book]
    if not selected:
        print(f"订单 {args.order} 里没有书目 {args.book}；可选：{', '.join(i['book_id'] for i in items)}",
              file=sys.stderr)
        return 2

    remaining = money(order["total"] - order["refunded"])
    result = {"order_id": order["id"], "amount": 0.0, "basis": "", "needs_approval": False,
              "refundable": True}

    if order["status"] in ("待发货", "已取消"):
        result.update(refundable=False, basis=f"订单状态是“{order['status']}”，没有可退的款项")
    elif args.reason == "quality" and days is not None and days > 60:
        result.update(refundable=False, basis=f"质量问题需在签收后 60 天内申请，这张订单已签收 {days} 天")
    elif args.reason == "changed_mind" and days is not None and days > 15:
        result.update(refundable=False, basis=f"非质量问题需在签收后 15 天内申请，这张订单已签收 {days} 天")
    elif args.reason == "changed_mind" and args.opened and any(i["category"] == "教辅" for i in selected):
        result.update(refundable=False, basis="教辅拆封后不退（非质量问题）")
    else:
        book_total = money(sum(i["price"] * i["qty"] * order["discount"] for i in selected))
        if args.reason == "quality" and len(selected) == len(items):
            amount = remaining
            if order["refunded"] > 0:
                result["basis"] = (f"质量问题退全款：书款 {money(book_total):.2f} + "
                                   f"运费 {money(order['shipping_fee']):.2f}；已退 {money(order['refunded']):.2f}，"
                                   f"本次最多退 {money(remaining):.2f}")
            else:
                result["basis"] = (f"质量问题退全款：书款 {money(book_total):.2f} + "
                                   f"运费 {money(order['shipping_fee']):.2f}")
        else:
            amount = min(book_total, remaining)
            if args.reason == "quality":
                result["basis"] = f"质量问题，只退所退书目：{money(book_total):.2f}"
            else:
                result["basis"] = f"非质量问题只退书款 {money(book_total):.2f}，运费 {money(order['shipping_fee']):.2f} 不退"
        result["amount"] = money(amount)
        result["needs_approval"] = amount > LIMIT
        if result["needs_approval"]:
            result["basis"] += f"；单笔超过 {LIMIT:.0f} 元，需要主管批准"

    print(json.dumps(result, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
