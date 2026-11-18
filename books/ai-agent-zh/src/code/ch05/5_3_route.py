# mode: run
# expect: T06.*退货退款
"""五张工单先过 submit_route，再查路由表。分类器没有业务工具，也不取订单详情。"""
import os
import sys
import pathlib

from openai import OpenAI

client = OpenAI()                                  # 读 OPENAI_BASE_URL、OPENAI_API_KEY
MODEL = os.environ.get("MODEL", "deepseek-flash")

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from ch05.common import ROUTES, add_usage, classify
from shop import tickets

wanted = ["T06", "T13", "T22", "T24", "T28"]
all_tickets = {t["id"]: t for t in tickets()}
total = {"prompt": 0, "completion": 0}

print("工单  分类        处理器          工具子集")
for tid in wanted:
    t = all_tickets[tid]
    category, reason, usage = classify(t["text"])
    total = add_usage(total, usage)
    route = ROUTES[category]
    print(f"{tid}  {category:<6}  {route['handler']:<14}  {', '.join(route['tools']) or '（无）'}")
    print(f"      依据：{reason}")

print(f"分类调用合计 token：{total['prompt']}+{total['completion']}（{len(wanted)} 次）")
