# mode: run
# timeout: 900
# expect: 全量
"""6 个精确问题：全部订单塞进 system（约 6.2 万 token）对比 3 个按需查询工具。"""
import json
import os

from openai import OpenAI

from harness import run
from shop import Store
from shop.tools import SUPPORT_PROMPT, support_tools

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")
store = Store()
box = support_tools(store, include=("find_customer", "get_customer_context", "get_order"))

orders = store.query("SELECT id, customer_id, created, status, total, refunded, address, shipping_fee, courier "
                     "FROM orders ORDER BY id")
for o in orders:
    o["items"] = [{"title": i["title"], "qty": i["qty"], "price": i["price"]}
                  for i in store.order(o["id"])["items"]]
ALL_ORDERS = json.dumps(orders, ensure_ascii=False, separators=(",", ":"))
STUFFED = [{"role": "system", "content": "你是青石书店的客服。下面是全部订单资料，只根据资料回答，不要猜。\n\n" + ALL_ORDERS}]

QUESTIONS = [
    ("C082 最近一张订单的订单号是什么？", "A1659"),
    ("C082 到目前为止累计退款多少元？只答数字。", "48"),
    ("订单 A1659 实付多少元？只答数字。", "82"),
    ("订单 A1679 现在是什么状态？", "待发货"),
    ("C101 最近一张订单用的是哪家快递？", "顺路"),
    ("订单 A1680 实付多少元？只答数字。", "680"),
]


def ok(answer, want):
    return want in answer.replace(" ", "").replace(",", "")


def ask_stuffed(question):
    resp = client.chat.completions.create(model=MODEL, messages=STUFFED + [{"role": "user", "content": question}],
                                          extra_body={"thinking": {"type": "disabled"}}, max_tokens=40)
    return resp.choices[0].message.content.strip(), resp.usage.prompt_tokens


def ask_fetch(question):
    res = run([{"role": "system", "content": SUPPORT_PROMPT}, {"role": "user", "content": question}],
              box, model=MODEL, max_turns=6, verbose=False,
              extra_body={"thinking": {"type": "disabled"}})
    return (res.answer or "").strip(), res.usage.get("prompt_tokens", 0)


print(f"全量订单 {len(orders)} 张，JSON {len(ALL_ORDERS):,} 字符")
total_a, total_b, good_a, good_b = 0, 0, 0, 0
for i, (q, want) in enumerate(QUESTIONS, 1):
    marks = []
    for label in ("A", "B"):
        for _ in range(2):
            ans, used = ask_stuffed(q) if label == "A" else ask_fetch(q)
            good = ok(ans, want)
            if label == "A":
                good_a += good
                total_a += used
            else:
                good_b += good
                total_b += used
            marks.append(f"{'全量' if label == 'A' else '按需'}{'✓' if good else '×'} {ans[:12]}")
    print(f"{i} {q[:20]:<22}{marks[0]:<18}{marks[1]:<18}{marks[2]:<18}{marks[3]}")
print(f"全量：{good_a}/12 正确，12 次共 prompt {total_a:,}，平均 {total_a // 12:,}/次")
print(f"按需：{good_b}/12 正确，12 次共 prompt {total_b:,}，平均 {total_b // 12:,}/次")
