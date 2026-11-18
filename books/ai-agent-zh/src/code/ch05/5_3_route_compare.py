# mode: run
# expect: 对比
"""同样五张工单：一个分类器 vs 一个大提示自己判断并写回复。只比 prompt token。"""
import json
import os
import sys
import pathlib

from openai import OpenAI

client = OpenAI()                                  # 读 OPENAI_BASE_URL、OPENAI_API_KEY
MODEL = os.environ.get("MODEL", "deepseek-flash")

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from ch05.common import Store, add_usage, chat, classify, facts_for, order_brief
from shop import tickets

BIG = """你是青石书店的客服。自己判断工单属于哪一类，再直接写一条回复。
类别有：物流查询、退货退款、修改地址、政策咨询、投诉建议、其他、越界。
只输出回复正文，不要解释你的分类。"""

wanted = ["T06", "T13", "T22", "T24", "T28"]
all_tickets = {t["id"]: t for t in tickets()}
store = Store()
route_total = {"prompt": 0, "completion": 0}
big_total = {"prompt": 0, "completion": 0}

print("工单  分类路由（只分类）                     一个大提示")
for tid in wanted:
    t = all_tickets[tid]
    category, _, route_u = classify(t["text"])
    route_total = add_usage(route_total, route_u)
    facts = facts_for(store, t["text"])
    payload = {"ticket": t["text"], "order": order_brief(facts["order"]), "policies": facts["policies"]}
    reply, big_u = chat([{"role": "system", "content": BIG},
                         {"role": "user", "content": json.dumps(payload, ensure_ascii=False)}], max_tokens=300)
    big_total = add_usage(big_total, big_u)
    print(f"{tid}  → {category:<6} {route_u['prompt']:>4} token          {big_u['prompt']:>4} token")
    print(f"      大提示的回复：{reply[:52]}")

print("对比（五张合计）")
print(f"  分类路由：prompt {route_total['prompt']}，completion {route_total['completion']}")
print(f"  一个大提示：prompt {big_total['prompt']}，completion {big_total['completion']}")
print(f"  只算分类这一步，大提示的 prompt 是路由的 {big_total['prompt'] / max(route_total['prompt'], 1):.1f} 倍")
