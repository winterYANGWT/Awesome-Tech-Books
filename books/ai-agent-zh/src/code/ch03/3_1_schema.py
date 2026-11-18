# 3.1 工具定义长什么样：从 Python 函数到 JSON Schema
# llm: none
# expect: "required"
# expect: 单笔超过 500 元不能直接退
import json

from shop import Store
from shop.tools import support_tools

box = support_tools(Store(), include=("get_order", "issue_refund"))
for name in ("get_order", "issue_refund"):
    print(f"--- {name} ---")
    print(json.dumps(box.tools[name].schema(), ensure_ascii=False, indent=2))
