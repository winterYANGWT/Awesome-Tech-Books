# 3.6 用 harness.run 重跑 T06，循环、错误处理和打印都已经在里面
# expect: issue_refund.*82
# expect: stop=answer
import os

from shop import Store
from shop.tools import SUPPORT_PROMPT, support_tools

from harness import run

MODEL = os.environ.get("MODEL", "deepseek-flash")
box = support_tools(Store(), include=("get_order", "search_policies", "issue_refund"))
messages = [
    {"role": "system", "content": SUPPORT_PROMPT},
    {"role": "user", "content": "收到的《大运河两千年》中间缺了二十多页，订单号 A1659，怎么处理？"},
]

result = run(messages, box, model=MODEL, max_turns=10, verbose=True)
print(f"stop={result.stop}  turns={result.turns}")
print(f"usage: {result.usage['prompt_tokens']}+{result.usage['completion_tokens']} token")
print(result.answer)
