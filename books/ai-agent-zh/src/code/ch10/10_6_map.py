"""把书店映射成 MCP server 的接口清单：9 个工具、5 份政策、2 个 prompt。"""
# llm: none
# expect: shop://policies/returns
# expect: issue_refund
# expect: destructiveHint=True

import json

from shop import Store
from shop.tools import support_tools

READ_ONLY = {"readOnlyHint": True, "destructiveHint": False, "openWorldHint": False}
DESTRUCTIVE = {"readOnlyHint": False, "destructiveHint": True, "openWorldHint": False}
ADDITIVE = {"readOnlyHint": False, "destructiveHint": False, "openWorldHint": False}

ANNOTATIONS = {
    "find_customer": READ_ONLY,
    "get_customer_context": READ_ONLY,
    "get_order": READ_ONLY,
    "search_books": READ_ONLY,
    "search_policies": READ_ONLY,
    "list_reviews": READ_ONLY,
    "issue_refund": DESTRUCTIVE,
    "update_address": DESTRUCTIVE,
    "escalate_to_supervisor": ADDITIVE,
}
RETURNS = {
    "find_customer": "customer_id, name, city, level",
    "get_customer_context": "customer_id, name, level, address, recent_orders",
    "get_order": "order_id, customer_id, created, status, items, shipping_fee, total_paid, refunded, address, shipment",
    "search_books": "book_id, title, author, category, price",
    "search_policies": "section, text",
    "list_reviews": "stars, text, date",
    "issue_refund": "order_id, amount, refunded_total, status",
    "update_address": "order_id, address",
    "escalate_to_supervisor": "ticket, status",
}
PROMPTS = {
    "refund_guidance": ["order_id", "amount"],
    "shipping_reply": ["order_id", "courier"],
}

store = Store()
tools = support_tools(store).tools
policies = store.policy_names()


def signature(name: str, schema: dict) -> str:
    required = set(schema.get("required", []))
    parts = []
    for prop, spec in schema.get("properties", {}).items():
        parts.append(prop + ("" if prop in required else "?"))
    return f"{name}({', '.join(parts)})"


def rough_tokens(text: str) -> int:
    cjk = sum(1 for ch in text if "\u4e00" <= ch <= "\u9fff")
    return round(cjk + (len(text) - cjk) / 4)


print("工具（9 个，模型控制）")
print("  名字 / 参数 / annotations / 返回字段")
for tool in tools.values():
    ann = ANNOTATIONS[tool.name]
    flags = " ".join(f"{key}={str(value)}" for key, value in ann.items())
    print(f"  {signature(tool.name, tool.parameters)}")
    print(f"    {flags}")
    print(f"    返回 {RETURNS[tool.name]}")

print()
print("资源（应用控制）：一个模板，5 份文档")
print("  shop://policies/{name}  mimeType=text/markdown")
for name in policies:
    text = store.policy(name)
    print(f"  shop://policies/{name}  ({len(text)} 字符，约 {rough_tokens(text)} token)")

print()
print("提示（用户控制）：2 个，参数都是字符串")
for name, args in PROMPTS.items():
    print(f"  {name}({', '.join(f'{arg}: str' for arg in args)})")

mcp_tools = [{"name": tool.name, "description": tool.description,
              "inputSchema": tool.parameters, "annotations": ANNOTATIONS[tool.name]}
             for tool in tools.values()]
schema_text = json.dumps(mcp_tools, ensure_ascii=False)
policy_text = "".join(store.policy(name) for name in policies)
print()
print(f"9 个工具的 schema：{len(schema_text)} 字符，粗估 {rough_tokens(schema_text)} token（每轮都要发）")
print(f"5 份政策全文：{len(policy_text)} 字符，粗估 {rough_tokens(policy_text)} token（按需读一份）")
print("这份清单就是第 11 章 server 的输入：名字、参数、返回字段和 annotations 都写死。")
