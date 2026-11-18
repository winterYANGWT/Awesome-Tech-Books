# mode: run
# llm: none
# expect: get_order
# expect: list_all_orders
# expect: required
"""把四个工具的 Tool.schema() 打出来：两个任务工具，两个按表包装的 CRUD 工具。"""
from __future__ import annotations

import json
import math
import re

from lib.bad_tools import crud_tools
from shop import Store
from shop.tools import support_tools


def rough_tokens(text: str) -> int:
    """粗略估算：一个汉字约 1 token，4 个 ASCII 字符约 1 token。"""
    han = len(re.findall(r"[\u4e00-\u9fff]", text))
    return math.ceil(han + (len(text) - han) / 4)


def show(tool) -> None:
    fn = tool.schema()["function"]
    desc = fn["description"].splitlines() if fn["description"] else []
    print(f"\n{fn['name']}")
    if desc:
        print(f"  description（{len(desc)} 行）:")
        for line in desc:
            print(f"    {line}")
    else:
        print("  description: （空）")
    print("  properties:")
    for name, prop in fn["parameters"]["properties"].items():
        extra = prop.get("description", "（无说明）")
        default = "，有默认值" if name not in fn["parameters"]["required"] else ""
        print(f"    {name}: {prop['type']}{default}  {extra}")
    text = json.dumps(fn, ensure_ascii=False)
    print(f"  required: {json.dumps(fn['parameters']['required'], ensure_ascii=False)}"
          f"   schema {len(text)} 字符，约 {rough_tokens(text)} token")


store = Store()
good = support_tools(store)
bad = crud_tools(store)
for t in (good.tools["get_order"], good.tools["search_policies"], good.tools["issue_refund"],
          bad.tools["list_all_orders"], bad.tools["get_customer"]):
    show(t)

good_text = json.dumps(good.schemas(), ensure_ascii=False)
bad_text = json.dumps(bad.schemas(), ensure_ascii=False)
print(f"\n9 个任务工具：{len(good_text)} 字符，约 {rough_tokens(good_text)} token")
print(f"24 个 CRUD 包装：{len(bad_text)} 字符，约 {rough_tokens(bad_text)} token"
      f"（描述是空的；补上字段说明会更大）")
