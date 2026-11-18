# 14.6 坏工具：read_all_orders 只给前 50 张，A1679 在末尾，agent 拿不到它。
# mode: fail
# expect: A1679
import os
import sys

from harness import ToolError, Toolbox, run, tool

from shop import Store

MODEL = os.environ.get("MODEL", "deepseek-flash")
store = Store()
CACHE: list[dict] = []


class OrderNotFound(Exception):
    """示例末尾的断言：A1679 没有被任何工具返回过。"""


# ANCHOR: bad_tool
@tool
def read_all_orders() -> dict:
    """一次读取全部订单（按订单号排序）。"""
    rows = store.query("SELECT id, status, total FROM orders ORDER BY id LIMIT 50")
    CACHE[:] = rows
    return {"orders": rows, "returned": len(rows), "total": 680}


@tool
def find_order(order_id: str) -> dict:
    """在上次 read_all_orders 的结果里按订单号查找。"""
    found = next((o for o in CACHE if o["id"] == order_id), None)
    if found is None:
        raise ToolError(f"OrderNotFound: {order_id} 不在返回的 {len(CACHE)} 条里")
    return found
# ANCHOR_END: bad_tool


PROMPT = """你是青石书店的客服。顾客问订单 A1679 发货了没有。
先用 read_all_orders 读取订单，再用 find_order 查 A1679，然后回答。只根据工具结果回答。"""


def main() -> None:
    messages = [{"role": "system", "content": PROMPT},
                {"role": "user", "content": "我的订单 A1679 发货了吗？"}]
    res = run(messages, Toolbox([read_all_orders, find_order]), model=MODEL, max_turns=4)
    print(f"\nagent 的回答：{res.answer}")
    print(f"read_all_orders 返回 {len(CACHE)} 条：{CACHE[0]['id']}…{CACHE[-1]['id']}，total=680")
    print(f"A1679 在返回结果里：{'是' if any(o['id'] == 'A1679' for o in CACHE) else '否'}")
    if "待发货" not in (res.answer or ""):
        raise OrderNotFound("A1679 不在返回的 50 条里")


if __name__ == "__main__":
    sys.exit(main())
