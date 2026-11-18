"""同一个工具箱，两版 search_policies 描述。A 组描述为空，B 组写清查什么和 query 格式。"""
from __future__ import annotations

from harness import Tool, ToolError, Toolbox
from shop.tools import support_tools

from shop import Store

DETAILED = ("查店铺政策条款：退货期限、质量问题、运费、发票、会员；不查订单和物流；"
            "query 用 2–4 个短关键词，例如“退货 期限”。")


def policy_tools(store: Store, detailed: bool) -> Toolbox:
    base = support_tools(store, include=("get_order", "search_policies"))
    p = base.tools["search_policies"]

    def search(query: str):
        if len(query) > 12:
            raise ToolError("query 太长；只写 2–4 个关键词，例如“退货 期限”")
        return p.fn(query)

    query = {"type": "string"}
    if detailed:
        query["description"] = "要查的问题，用 2–4 个短关键词，例如“退货 期限”"
    params = {"type": "object", "properties": {"query": query}, "required": ["query"]}
    tool = Tool("search_policies", DETAILED if detailed else "", params, search)
    return Toolbox([tool, base.tools["get_order"]])
