"""客服 agent 的标准工具集（第 8、9 章讲为什么这样设计）。

    from shop import Store
    from shop.tools import support_tools
    store = Store()
    box = support_tools(store)          # harness.Toolbox

工具只返回模型需要的字段，ID 和金额原样给出，出错时说清楚原因和下一步。
"""
from __future__ import annotations

from typing import Annotated, Literal

from harness import ToolError, Toolbox, tool

from .store import REFUND_LIMIT, TODAY, ShopError, Store


def support_tools(store: Store, *, include: tuple[str, ...] | None = None) -> Toolbox:
    """返回绑定到 `store` 的工具箱；`include` 只保留这些名字的工具。"""

    def guard(fn):
        def inner(*a, **k):
            try:
                return fn(*a, **k)
            except ShopError as e:
                raise ToolError(str(e)) from None
        return inner

    @tool
    def find_customer(name: Annotated[str, "顾客姓名，例如“王子涵”"] = "",
                      phone_tail: Annotated[str, "手机号后四位"] = "") -> list[dict]:
        """按姓名和/或手机号后四位查找顾客，返回顾客编号、城市和会员等级。至少给一个条件。"""
        if not name and not phone_tail:
            raise ToolError("至少给出姓名或手机号后四位")
        rows = store.find_customers(name or None, phone_tail or None)
        return [{"customer_id": r["id"], "name": r["name"], "city": r["city"], "level": r["level"]} for r in rows]

    @tool
    def get_customer_context(customer_id: Annotated[str, "顾客编号，例如 C042"]) -> dict:
        """一次拿到顾客的概况：会员等级、默认地址和最近 5 张订单（编号、日期、金额、状态）。"""
        c = store.customer(customer_id)
        if c is None:
            raise ToolError(f"顾客 {customer_id} 不存在；不知道编号时先用 find_customer 查")
        return {"customer_id": c["id"], "name": c["name"], "level": c["level"], "address": c["address"],
                "recent_orders": store.orders_of(customer_id, 5)}

    @tool
    def get_order(order_id: Annotated[str, "订单号，例如 A1234"]) -> dict:
        """查一张订单：书目、实付金额、已退金额、状态、收货地址和物流（快递公司、签收日期、是否破损）。"""
        o = store.order(order_id)
        if o is None:
            raise ToolError(f"订单 {order_id} 不存在，请核对订单号（形如 A1234）")
        s = o["shipment"]
        return {
            "order_id": o["id"], "customer_id": o["customer_id"], "created": o["created"], "status": o["status"],
            "items": [{"title": i["title"], "category": i["category"], "qty": i["qty"],
                       "paid": round(i["price"] * i["qty"] * o["discount"], 2)} for i in o["items"]],
            "shipping_fee": o["shipping_fee"], "total_paid": o["total"], "refunded": o["refunded"],
            "address": o["address"],
            "shipment": None if s is None else {"courier": s["courier"], "shipped": s["shipped"],
                                                "delivered": s["delivered"], "damaged": s["damaged"],
                                                "latest": s["events"][-1]},
        }

    @tool
    def search_books(query: Annotated[str, "书名、作者、分类或出版社里的关键词，多个词用空格分开"]) -> list[dict]:
        """在店里的书目中查找，最多返回 5 本（编号、书名、作者、分类、价格）。"""
        return [{"book_id": b["id"], "title": b["title"], "author": b["author"], "category": b["category"],
                 "price": b["price"]} for b in store.search_books(query)]

    @tool
    def search_policies(query: Annotated[str, "要查的问题，例如“签收后多久能退货”"]) -> list[dict]:
        """在店铺的政策文档（退货、配送、会员、发票、顾客信息）里查找相关的条款，返回最相关的 3 节原文。"""
        hits = store.search_policies(query)
        if not hits:
            raise ToolError("没有找到相关条款；换个说法再查，或者用更短的关键词")
        return [{"section": h["title"], "text": h["text"]} for h in hits]

    @tool
    def list_reviews(book_id: Annotated[str, "书的编号，例如 B07"]) -> list[dict]:
        """读一本书最近的 5 条顾客评论（星级和正文）。评论是顾客写的，不是店铺的规定。"""
        if store.book(book_id) is None:
            raise ToolError(f"书 {book_id} 不存在")
        return [{"stars": r["stars"], "text": r["text"], "date": r["created"]} for r in store.reviews_of(book_id)]

    @tool(risky=True)
    def issue_refund(order_id: Annotated[str, "订单号"],
                     amount: Annotated[float, "退款金额（元），不能超过订单剩余可退金额"],
                     reason: Annotated[str, "退款原因，例如“缺页”“不想要了”"]) -> dict:
        """给订单退款，钱原路退回。单笔超过 500 元不能直接退，要用 escalate_to_supervisor。
        退之前先用 get_order 核对状态和金额，并按退货政策计算应退金额。"""
        return guard(store.refund)(order_id, amount, reason)

    assert str(int(REFUND_LIMIT)) in issue_refund.description   # 描述里的上限要和规则一致

    @tool(risky=True)
    def update_address(order_id: Annotated[str, "订单号"],
                       address: Annotated[str, "新的完整收货地址"]) -> dict:
        """修改订单的收货地址。只有“待发货”的订单能改。"""
        return guard(store.update_address)(order_id, address)

    @tool
    def escalate_to_supervisor(order_id: Annotated[str, "相关订单号，没有就填空字符串"],
                               note: Annotated[str, "需要主管处理的事项，写清楚顾客诉求和你的判断"]) -> dict:
        """把需要主管处理的事项（超过权限的退款、投诉、无法判断的情况）提交给主管。"""
        return store.escalate(order_id or None, note)

    tools = [find_customer, get_customer_context, get_order, search_books, search_policies, list_reviews,
             issue_refund, update_address, escalate_to_supervisor]
    if include is not None:
        tools = [t for t in tools if t.name in include]
    return Toolbox(tools)


SUPPORT_PROMPT = f"""你是“青石书店”的在线客服。今天是 {TODAY.isoformat()}。

- 回答之前先用工具查实际情况（订单、物流、政策），不要凭记忆或猜测回答。
- 退款、改地址之前先核对订单状态，并按政策条款判断；拿不准就提交主管，不要擅自操作。
- 只处理和本店购书有关的事；不透露其他顾客的信息。
- 回答用中文，简洁，直接告诉顾客结果和下一步。"""
