"""反面教材：把 9 个客服任务按数据库表拆成 24 个 CRUD 包装。

这些工具都能用，但只按表名、列名命名，描述基本是空的。第 8.1、8.2 节拿它们和
`shop.tools.support_tools` 的 9 个任务工具对比。不要在自己的项目里照抄。
"""
from __future__ import annotations

from harness import ToolError, Toolbox, tool

from shop import ShopError, Store


def crud_tools(store: Store) -> Toolbox:
    """24 个按表拆开的工具，绑定到 `store`。"""

    def guard(fn):
        def inner(*a, **k):
            try:
                return fn(*a, **k)
            except ShopError as e:
                raise ToolError(str(e)) from None
        return inner

    # ---- books（3）---------------------------------------------------------
    @tool
    def list_books(limit: int = 50, offset: int = 0) -> list[dict]:
        limit = min(max(limit, 1), 50)
        return store.query("SELECT id, title, author, category, price FROM books LIMIT ? OFFSET ?", limit, offset)

    @tool
    def get_book(book_id: str) -> dict:
        b = store.book(book_id)
        if b is None:
            raise ToolError(f"book {book_id} not found")
        return b

    @tool
    def search_books_by_title(title: str, limit: int = 50) -> list[dict]:
        return store.query("SELECT id, title, price FROM books WHERE title LIKE ? LIMIT ?", f"%{title}%", min(limit, 50))

    # ---- customers（4）-----------------------------------------------------
    @tool
    def list_customers(limit: int = 50, offset: int = 0) -> list[dict]:
        limit = min(max(limit, 1), 50)
        return store.query("SELECT id, name, city, level, phone_tail FROM customers LIMIT ? OFFSET ?", limit, offset)

    @tool
    def get_customer(customer_id: str) -> dict:
        c = store.customer(customer_id)
        if c is None:
            raise ToolError(f"customer {customer_id} not found")
        return c

    @tool
    def find_customer_by_name(name: str) -> list[dict]:
        return store.query("SELECT id, name, city, level, phone_tail FROM customers WHERE name = ?", name)

    @tool
    def find_customer_by_phone(phone_tail: str) -> list[dict]:
        return store.query("SELECT id, name, city, level, phone_tail FROM customers WHERE phone_tail = ?", phone_tail)

    # ---- orders（3）-------------------------------------------------------
    @tool
    def list_all_orders(customer_id: str = "", status: str = "", limit: int = 50, offset: int = 0) -> list[dict]:
        sql, args = "SELECT id, customer_id, created, status, total, refunded FROM orders WHERE 1", []
        if customer_id:
            sql, args = sql + " AND customer_id = ?", args + [customer_id]
        if status:
            sql, args = sql + " AND status = ?", args + [status]
        return store.query(sql + " LIMIT ? OFFSET ?", *args, min(limit, 50), offset)

    @tool
    def list_orders_by_date(start: str, end: str, limit: int = 50) -> list[dict]:
        return store.query("SELECT id, customer_id, created, status, total, refunded FROM orders "
                           "WHERE created BETWEEN ? AND ? LIMIT ?", start, end, min(limit, 50))

    @tool
    def list_orders_by_status(status: str, limit: int = 50) -> list[dict]:
        return store.query("SELECT id, customer_id, created, status, total FROM orders WHERE status = ? LIMIT ?",
                           status, min(limit, 50))

    # ---- order_items（2）--------------------------------------------------
    @tool
    def list_order_items(order_id: str = "", limit: int = 50, offset: int = 0) -> list[dict]:
        sql, args = "SELECT order_id, book_id, qty, price FROM order_items WHERE 1", []
        if order_id:
            sql, args = sql + " AND order_id = ?", args + [order_id]
        return store.query(sql + " LIMIT ? OFFSET ?", *args, min(limit, 50), offset)

    @tool
    def get_order_item(order_id: str, book_id: str) -> dict:
        rows = store.query("SELECT order_id, book_id, qty, price FROM order_items "
                           "WHERE order_id = ? AND book_id = ?", order_id, book_id)
        if not rows:
            raise ToolError("order item not found")
        return rows[0]

    # ---- shipments（3）----------------------------------------------------
    @tool
    def list_shipments(limit: int = 50) -> list[dict]:
        limit = min(max(limit, 1), 50)          # 分页上限 50，且没有 offset
        return store.query("SELECT order_id, courier, shipped, delivered, damaged FROM shipments LIMIT ?", limit)

    @tool
    def list_damaged_shipments(limit: int = 50) -> list[dict]:
        return store.query("SELECT order_id, courier, shipped, delivered FROM shipments "
                           "WHERE damaged = 1 LIMIT ?", min(limit, 50))

    @tool
    def count_shipments() -> dict:
        return store.query("SELECT COUNT(*) AS n FROM shipments")[0]

    # ---- returns（3）------------------------------------------------------
    @tool
    def list_returns(order_id: str = "", limit: int = 50) -> list[dict]:
        if order_id:
            return store.query("SELECT * FROM returns WHERE order_id = ? LIMIT ?", order_id, limit)
        return store.query("SELECT * FROM returns LIMIT ?", min(limit, 50))

    @tool
    def get_return(id: str) -> dict:
        rows = store.query("SELECT * FROM returns WHERE id = ?", id)
        if not rows:
            raise ToolError(f"return {id} not found")
        return rows[0]

    @tool
    def create_return(order_id: str, book_id: str, reason: str) -> dict:
        return store.query("SELECT 1 AS ok")[0] | {"order_id": order_id, "book_id": book_id, "reason": reason,
                                                   "note": "只登记退货申请，不退款"}

    # ---- policies（2）-----------------------------------------------------
    @tool
    def list_policies() -> list[str]:
        return store.policy_names()

    @tool
    def get_policy(name: str) -> str:
        return store.policy(name)

    # ---- actions（1）------------------------------------------------------
    @tool
    def list_actions(limit: int = 50) -> list[dict]:
        return store.query("SELECT * FROM actions LIMIT ?", limit)

    # ---- 会改数据的两个包装（2）---------------------------------------------
    @tool
    def create_refund(order_id: str, amount: float, reason: str) -> dict:
        return guard(store.refund)(order_id, amount, reason)

    @tool
    def update_order_address(order_id: str, address: str) -> dict:
        return guard(store.update_address)(order_id, address)

    # ---- reviews（1）------------------------------------------------------
    @tool
    def list_reviews(book_id: str, limit: int = 50) -> list[dict]:
        return store.query("SELECT id, book_id, stars, text, created FROM reviews WHERE book_id = ? LIMIT ?",
                           book_id, min(limit, 50))

    return Toolbox([list_books, get_book, search_books_by_title,
                    list_customers, get_customer, find_customer_by_name, find_customer_by_phone,
                    list_all_orders, list_orders_by_date, list_orders_by_status,
                    list_order_items, get_order_item,
                    list_shipments, list_damaged_shipments, count_shipments,
                    list_returns, get_return, create_return,
                    list_policies, get_policy, list_actions,
                    create_refund, update_order_address, list_reviews])
