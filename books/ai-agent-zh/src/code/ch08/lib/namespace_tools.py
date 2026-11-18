"""两个本地工具服务：书店的查书、CRM 的查顾客。

A 组用裸名 `search` / `find`；B 组加双下划线服务前缀。两组的描述和参数**完全一样**，
只有名字不同，所以选择全靠名字。
"""
from __future__ import annotations

from harness import Toolbox, tool

from shop import Store


def namespace_tools(store: Store, prefixed: bool) -> Toolbox:
    shop_name = "shop__search_books" if prefixed else "search"
    crm_name = "crm__find_customers" if prefixed else "find"

    @tool(name=shop_name)
    def shop_search(query: str) -> list[dict]:
        "查找记录。"
        return [{"book_id": b["id"], "title": b["title"], "price": b["price"]}
                for b in store.search_books(query)]

    @tool(name=crm_name)
    def crm_find(query: str) -> list[dict]:
        "查找记录。"
        return [{"customer_id": c["id"], "name": c["name"], "level": c["level"]}
                for c in store.find_customers()
                if query in c["name"] or query == c["phone_tail"]]

    return Toolbox([shop_search, crm_find])
