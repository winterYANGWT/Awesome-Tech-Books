"""强制工具调用：让模型把结果填进 submit_ticket 的参数。"""
# expect: submit_ticket
import json
import os
from typing import Annotated, Literal

from openai import OpenAI

from harness import tool
from shop import tickets

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")

IDS = ["T01", "T06", "T09", "T12", "T13", "T14", "T22", "T24", "T27", "T28"]
CATEGORIES = ["物流查询", "退货退款", "修改地址", "政策咨询", "投诉建议", "其他", "越界"]


@tool
def submit_ticket(
    category: Literal["物流查询", "退货退款", "修改地址", "政策咨询", "投诉建议", "其他", "越界"],
    order_id: Annotated[str, "订单号，形如 A1234；工单里没有就填空字符串"],
    amount: Annotated[float, "工单涉及的金额（元），没有就填 0"],
    reason: Annotated[str, "一句话说明分类理由"],
) -> dict:
    """提交一张工单的分类结果。只收结果，不退款、不改地址、不发消息。"""
    return {"category": category, "order_id": order_id, "amount": amount, "reason": reason}


SYSTEM = """你是青石书店的客服助理。请调用 submit_ticket 提交这张工单的分类结果。
category 从工具参数的枚举里选；order_id 没有就填空字符串；amount 没有就写 0；reason 写一句话。
不要用普通文本回答。"""


def ask(text: str):
    resp = client.chat.completions.create(
        model=MODEL,
        messages=[{"role": "system", "content": SYSTEM}, {"role": "user", "content": text}],
        tools=[submit_ticket.schema()],
        tool_choice={"type": "function", "function": {"name": "submit_ticket"}},
        max_tokens=600,
        extra_body={"thinking": {"type": "disabled"}},
    )
    return resp.choices[0].message


def main() -> None:
    by_id = {t["id"]: t for t in tickets()}
    rows = []
    for tid in IDS:
        t = by_id[tid]
        msg = ask(t["text"])
        if not msg.tool_calls:
            raise RuntimeError(f"{tid}: 模型没有调用 submit_ticket（content={msg.content!r}）")
        call = msg.tool_calls[0]
        args = json.loads(call.function.arguments)
        cat = args.get("category")
        enum_ok = cat in CATEGORIES
        label_ok = cat == t["category"]
        rows.append((tid, call.function.name, cat, args.get("order_id"), args.get("amount"), enum_ok, label_ok))
        print(f"{tid}  {call.function.name}  {cat:<6} {args.get('order_id') or '—':<6} "
              f"{'一致' if label_ok else '不一致'}")

    print(f"工具名={rows[0][1]}，arguments 可解析 {len(rows)}/{len(rows)}，"
          f"category 在枚举 {sum(r[5] for r in rows)}/{len(rows)}，"
          f"类别一致 {sum(r[6] for r in rows)}/{len(rows)}")


if __name__ == "__main__":
    main()
