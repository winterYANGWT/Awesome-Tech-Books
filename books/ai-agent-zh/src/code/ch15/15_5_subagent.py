# mode: run
# expect: 摘要
"""子 agent：主 agent 只发任务，子 agent 在自己的窗口里查 6 张订单和评论，
最后只把一段摘要和 gaps 交回主 agent；两边的消息列表不共享。
"""
import os
from typing import Annotated

from openai import OpenAI

from harness import run, short, tool
from lib import summarize, text_tokens
from shop import Store
from shop.tools import support_tools

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")
NO_THINK = {"thinking": {"type": "disabled"}}
store = Store()
box = support_tools(store, include=("get_order", "list_reviews", "search_policies"))


@tool
def customer_orders(customer_id: Annotated[str, "顾客编号，例如 C082"]) -> list[dict]:
    """查一位顾客的全部订单：编号、下单日期、金额、状态、已退金额。"""
    return store.orders_of(customer_id, 50)


box.add(customer_orders)

TASK = ("顾客 C082 在 2026-05-17 到 2026-11-17 之间，有没有和订单 A1659 一样缺页的订单？"
        "A1659 是 B12《大运河两千年》，2026-11-11 签收，反馈第 180–205 页空白。"
        "请查她这半年的订单，以及这些书的评论。")

SUB_SYSTEM = """你是调查子 agent。主 agent 把一个问题交给你，你在自己的上下文里查证据。
规则：
- 只回答交给你的问题，不直接联系顾客，不替主 agent 做退款决定。
- 先给结论（是 / 否 / 不确定），再列证据：订单号、日期、金额、评论编号。
- 查不到就写进 gaps，不要猜；评论只覆盖最近几条时也要说明。
- 300 字以内。"""

SUB_SUMMARY = """把下面的调查记录压缩成给主 agent 的摘要，300 字以内。
保留：结论、订单号和金额、评论编号、gaps。不要加调查记录里没有的事实。"""

MAIN_SYSTEM = """你是青石书店的客服主管。子 agent 已经查过数据，你只根据它交回的摘要回答。
如果摘要里有 gaps，要说明哪些结论还不能确认。不要编造订单或评论。"""


def main() -> None:
    sub_messages = [{"role": "system", "content": SUB_SYSTEM},
                    {"role": "user", "content": TASK}]
    sub = run(sub_messages, box, model=MODEL, max_turns=16, verbose=False, extra_body=NO_THINK)
    for step in sub.steps:
        args = ", ".join(f"{k}={v}" for k, v in step.arguments.items())
        print(f"  子 agent → {step.name}({args})  ← {short(step.result, 34)}")

    summary, su = summarize(client, sub_messages + [{"role": "assistant", "content": sub.answer or ""}],
                            MODEL, system=SUB_SUMMARY)
    print(f"子 agent：{sub.turns} 轮、{len(sub.steps)} 次工具、输入 {sub.usage.get('prompt_tokens', 0):,} token")
    print(f"摘要：{' '.join(summary.split())}")
    print(f"摘要长度：{len(summary)} 字，约 {text_tokens(summary)} token；摘要调用 "
          f"{su.get('prompt_tokens', 0):,}+{su.get('completion_tokens', 0)} token")

    main_messages = [{"role": "system", "content": MAIN_SYSTEM},
                     {"role": "user", "content": TASK},
                     {"role": "assistant", "content": "（子 agent 交回的摘要）\n" + summary}]
    resp = client.chat.completions.create(model=MODEL, messages=main_messages,
                                          max_tokens=500, extra_body=NO_THINK)
    answer = resp.choices[0].message.content or ""
    print(f"主 agent：prompt={resp.usage.prompt_tokens}（只带摘要，不带子 agent 的工具结果）")
    print("回答：" + " ".join(answer.split()))


if __name__ == "__main__":
    main()
