# mode: run
# timeout: 300
# expect: list_all_orders
"""只跑 T01 一次，打开 verbose，看坏工具集把哪些行塞进了上下文。"""
from __future__ import annotations

from lib.bad_tools import crud_tools
from harness import run
from shop import Store, tickets
from shop.tools import SUPPORT_PROMPT

ticket = next(t for t in tickets() if t["id"] == "T01")
store = Store()
messages = [{"role": "system", "content": SUPPORT_PROMPT},
            {"role": "user", "content": ticket["text"]}]
res = run(messages, crud_tools(store), max_turns=8, verbose=True)

print(f"\n最终回答：{res.answer}")
print(f"提到迅达快递：{'是' if '迅达快递' in (res.answer or '') else '否'}")
print(f"轮数 {res.turns}，输入 token {res.usage.get('prompt_tokens', 0)}")
