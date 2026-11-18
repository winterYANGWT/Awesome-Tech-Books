"""JSON 模式：格式稳了，枚举和字段仍要自己查。"""
# expect: 模式=json_object
import json
import os
import re

from openai import OpenAI

from shop import tickets

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")

IDS = ["T01", "T06", "T09", "T12", "T13", "T14", "T22", "T24", "T27", "T28"]
CATEGORIES = ["物流查询", "退货退款", "修改地址", "政策咨询", "投诉建议", "其他", "越界"]

SYSTEM = """你是青石书店的客服助理。把工单分类，只输出一个 json 对象，不要代码块、不要解释。
json 的格式示例：{"category": "退货退款", "order_id": "A1659"}
category 只能是这七个值之一：物流查询、退货退款、修改地址、政策咨询、投诉建议、其他、越界。
order_id 写成 A 加四位数字；工单里没有订单号就填 null。"""


def ask(text: str) -> tuple[str, str]:
    resp = client.chat.completions.create(
        model=MODEL,
        messages=[{"role": "system", "content": SYSTEM}, {"role": "user", "content": text}],
        response_format={"type": "json_object"},
        max_tokens=600,
    )
    return resp.choices[0].message.content or "", resp.choices[0].finish_reason


def main() -> None:
    by_id = {t["id"]: t for t in tickets()}
    print("模式=json_object")
    rows = []
    for tid in IDS:
        t = by_id[tid]
        content, finish = ask(t["text"])
        empty = not content.strip()
        try:
            obj = json.loads(content) if not empty else {}
        except json.JSONDecodeError:
            obj = {}
        cat = obj.get("category")
        order = obj.get("order_id")
        expect_order = bool(re.search(r"A\d{4}", t["text"]))
        field_ok = isinstance(obj, dict) and "category" in obj and (order is not None or not expect_order)
        enum_ok = cat in CATEGORIES
        label_ok = cat == t["category"]
        rows.append({"id": tid, "empty": empty, "enum": enum_ok, "field": field_ok,
                     "label": label_ok, "cat": cat, "order": order, "finish": finish})
        print(f"{tid}  {str(cat):<6} {str(order):<7} {finish}")

    def count(key: str) -> int:
        return sum(bool(r[key]) for r in rows)

    print(f"可解析 {sum(not r['empty'] and r['cat'] is not None for r in rows)}/{len(rows)}，"
          f"空内容 {count('empty')}，枚举外 {sum(not r['enum'] for r in rows)}，"
          f"缺字段 {sum(not r['field'] for r in rows)}，类别一致 {count('label')}/{len(rows)}")
    off = [r for r in rows if not r["enum"]]
    if off:
        print("枚举外的值：" + "、".join(f"{r['id']}={r['cat']}" for r in off))


if __name__ == "__main__":
    main()
