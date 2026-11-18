#!/usr/bin/env python3
"""某个月的运营汇总。stdout 只放一行 CSV，诊断走 stderr。

    python scripts/report.py --month 2026-09

退出码：0 有数据；2 月份格式不对；3 该月没有订单。
"""
from __future__ import annotations

import argparse
import csv
import io
import pathlib
import re
import sys
from collections import Counter

# 让脚本从任意目录运行都能 import shop：src/code/ch16/<示例>/<skill>/scripts/
CODE = pathlib.Path(__file__).resolve().parents[4]
sys.path.insert(0, str(CODE))

from shop import Store  # noqa: E402

HEADER = ["订单数", "实付总额", "退货订单数", "退货率", "前三个退货原因", "快递破损数"]


def main() -> int:
    p = argparse.ArgumentParser(description="生成某个月的运营汇总 CSV；只读。",
                               epilog="退出码：0 有数据；2 月份格式不对；3 该月没有订单。")
    p.add_argument("--month", required=True, help="月份，格式 YYYY-MM，例如 2026-09")
    args = p.parse_args()
    if not re.fullmatch(r"\d{4}-\d{2}", args.month):
        print(f"--month 必须是 YYYY-MM，收到的是 {args.month!r}", file=sys.stderr)
        return 2

    store = Store()
    rows = store.query("SELECT * FROM orders WHERE created LIKE ? ORDER BY id", args.month + "-%")
    if not rows:
        print(f"{args.month} 没有订单", file=sys.stderr)
        return 3
    valid = [o for o in rows if o["status"] != "已取消"]
    ids = {o["id"] for o in valid}

    returns = store.query("SELECT order_id, reason FROM returns")
    month_returns = [r for r in returns if r["order_id"] in ids]
    return_orders = {r["order_id"] for r in month_returns}
    reasons = Counter(r["reason"] for r in month_returns)
    top = sorted(reasons.items(), key=lambda kv: (-kv[1], kv[0]))[:3]

    damaged = store.query(
        "SELECT COUNT(*) AS n FROM shipments s JOIN orders o ON o.id = s.order_id "
        "WHERE o.created LIKE ? AND o.status != '已取消' AND s.damaged = 1", args.month + "-%")[0]["n"]

    rate = len(return_orders) / len(valid) * 100 if valid else 0.0
    print(f"[report] month={args.month} orders={len(rows)} canceled={len(rows) - len(valid)} "
          f"valid={len(valid)} returns={len(return_orders)}", file=sys.stderr)

    buf = io.StringIO()
    w = csv.writer(buf)
    w.writerow(HEADER)
    w.writerow([len(valid), f"{sum(o['total'] for o in valid):.2f}", len(return_orders),
                f"{rate:.1f}%", "；".join(f"{k} {v}" for k, v in top), damaged])
    sys.stdout.write(buf.getvalue())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
