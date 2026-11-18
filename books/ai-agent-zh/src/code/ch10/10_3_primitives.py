"""三种原语各归谁控制：模型调工具，应用读资源，用户选提示。"""
# llm: none
# expect: tools/list
# expect: resources/templates/list
# expect: resources/read
# expect: prompts/get
# expect: required=True

import unicodedata

from shop import Store
from shop.tools import support_tools

RESOURCE_TEMPLATE = "shop://policies/{name}"
PROMPTS = {
    "refund_guidance": {
        "title": "退款处理话术",
        "description": "按订单和退款原因生成给顾客的回复",
        "arguments": [("order_id", "订单号，例如 A1680", True),
                      ("amount", "应退金额（元），例如 680.00；协议里传字符串", True)],
    },
    "shipping_reply": {
        "title": "物流回复模板",
        "description": "按订单和快递公司生成物流查询的回复",
        "arguments": [("order_id", "订单号", True),
                      ("courier", "快递公司，例如“顺路快递”", True)],
    },
}


def width(text: str) -> int:
    return sum(2 if unicodedata.east_asian_width(ch) in "WF" else 1 for ch in text)


def pad(text: str, cells: int) -> str:
    return text + " " * max(0, cells - width(text))


store = Store()
tools = support_tools(store).tools
policies = store.policy_names()

print(pad("原语", 12) + pad("谁控制", 8) + pad("方法", 58) + "书店里的对象")
rows = [
    ("tools", "模型", "tools/list, tools/call", f"{len(tools)} 个客服工具"),
    ("resources", "应用", "resources/list, resources/templates/list, resources/read", f"{len(policies)} 份政策文档"),
    ("prompts", "用户", "prompts/list, prompts/get", f"{len(PROMPTS)} 个客服话术"),
]
for row in rows:
    print(pad(row[0], 12) + pad(row[1], 8) + pad(row[2], 58) + row[3])

print()
print("9 个工具（模型决定调用）：")
print("  " + ", ".join(tools))
print("5 份政策（应用决定加载，一个 URI 模板）：")
print(f"  resources/templates/list → {RESOURCE_TEMPLATE}")
print("  resources/read 返回 contents：uri + mimeType=text/markdown + text")
for name in policies:
    print(f"  shop://policies/{name}")
print("2 个 prompt（用户决定选择）：")
for name, spec in PROMPTS.items():
    print(f"  {name}  — {spec['title']}")
    for arg, _, required in spec["arguments"]:
        print(f"    {arg}: str, required={required}")
print()
print("5 份政策不在上面 9 个工具里：它们是资源，不是动作。")
