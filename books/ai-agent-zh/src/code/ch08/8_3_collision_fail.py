# mode: fail
# llm: none
# expect: 被覆盖
"""两个 server 都叫 search，放进同一个 Toolbox 时后者静默覆盖前者。"""
from __future__ import annotations

from harness import Tool, Toolbox

shop_search = Tool("search", "查书店的书。", {"type": "object", "properties": {"query": {"type": "string"}}},
                   lambda query: [{"title": "大运河两千年"}])
crm_search = Tool("search", "查 CRM 的顾客。", {"type": "object", "properties": {"name": {"type": "string"}}},
                  lambda name: [{"name": "周子涵"}])

box = Toolbox([shop_search, crm_search])
print(f"放进 2 个工具，工具箱里有 {len(box.tools)} 个：{list(box.tools)}")
print(f"前一个还在吗：{'查书店的书。' in [t.description for t in box.tools.values()]}")
assert len(box.tools) == 2, f"同名工具 search 被覆盖：期望 2 个，实际 {len(box.tools)} 个"
