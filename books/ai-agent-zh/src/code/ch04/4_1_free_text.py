"""自由文本 + 正则：能解析多少张工单？"""
# expect: 10 张
import os
import re

from openai import OpenAI

from shop import tickets

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")

IDS = ["T01", "T06", "T09", "T12", "T13", "T14", "T22", "T24", "T27", "T28"]
CATEGORIES = ["物流查询", "退货退款", "修改地址", "政策咨询", "投诉建议", "其他", "越界"]

SYSTEM = """你是青石书店的客服助理。读一张工单，用一句话回答：类别是什么？订单号是什么？
类别用这七个词之一：物流查询、退货退款、修改地址、政策咨询、投诉建议、其他、越界。
不要解释，不要寒暄。"""


def ask(text: str) -> str:
    resp = client.chat.completions.create(
        model=MODEL,
        messages=[{"role": "system", "content": SYSTEM}, {"role": "user", "content": text}],
        max_tokens=600,
    )
    return resp.choices[0].message.content or ""


def main() -> None:
    by_id = {t["id"]: t for t in tickets()}
    print(f"{len(IDS)} 张工单")
    rows = []
    for tid in IDS:
        t = by_id[tid]
        raw = ask(t["text"])
        m = re.search(r"类别[:：]\s*([^\s，。；,;、]+)", raw)
        parsed = m.group(1).strip("。，,.;；") if m else None
        order = " ".join(raw.split())
        found_order = bool(re.search(r"A\d{4}", raw))
        enum_ok = parsed in CATEGORIES
        order_ok = found_order or not re.search(r"A\d{4}", t["text"])
        label_ok = parsed == t["category"]
        # 这一列只查回答里有没有“主管”“拆封”，不是业务正确率：
        # 提示没有要求模型给处置建议，真正的业务校验在 4.4。
        biz_ok = label_ok and (tid != "T13" or "主管" in raw) and (tid != "T14" or "拆封" in raw)
        rows.append({"id": tid, "raw": raw, "parsed": parsed, "enum": enum_ok, "order": order_ok,
                     "label": label_ok, "biz": biz_ok})
        print(f"{tid}  {parsed or '—':<6}  {'✓' if found_order else '—'}  {order}")

    def count(key: str) -> int:
        return sum(r[key] for r in rows)

    print(f"可解析 {sum(r['parsed'] is not None for r in rows)}/{len(rows)}，"
          f"值在枚举 {count('enum')}/{len(rows)}，订单号齐全 {count('order')}/{len(rows)}，"
          f"类别一致 {count('label')}/{len(rows)}，业务正确 {count('biz')}/{len(rows)}")
    bad = [r for r in rows if r["parsed"] is None]
    if bad:
        print(f"格式失败 {len(bad)} 条：")
        for r in bad:
            print(f"  {r['id']} 原文：{' '.join(r['raw'].split())}")
    off = [r for r in rows if r["parsed"] is not None and not r["enum"]]
    if off:
        print(f"值不在枚举 {len(off)} 条：" + "、".join(f"{r['id']}={r['parsed']}" for r in off))


if __name__ == "__main__":
    main()
