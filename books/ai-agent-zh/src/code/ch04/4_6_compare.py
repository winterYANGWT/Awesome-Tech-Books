"""20 张工单 × 3 种输出方式：同一批工单、同一模型、同一提示骨架。"""
# expect: 强制工具
# timeout: 300
import json
import os
import re
from typing import Annotated, Literal

from openai import OpenAI

from harness import tool
from shop import tickets

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")
IDS = ["T01", "T02", "T05", "T06", "T08", "T09", "T12", "T13", "T14", "T15",
       "T17", "T19", "T22", "T24", "T25", "T26", "T27", "T28", "T29", "T30"]

BASE = """你是青石书店的客服助理。读一张工单，判断它属于哪一类。
类别只能从这七个里选：物流查询、退货退款、修改地址、政策咨询、投诉建议、其他、越界。"""

FREE = BASE + "用一句话回答：类别是什么？订单号是什么？不要解释。"
JSON_PROMPT = BASE + """只输出一个 json 对象，不要代码块、不要解释。
json 的格式示例：{"category": "退货退款", "order_id": "A1659"}
order_id 写成 A 加四位数字；工单里没有订单号就填 null。"""


# ANCHOR: tool
@tool
def submit_ticket(
    category: Literal["物流查询", "退货退款", "修改地址", "政策咨询", "投诉建议", "其他", "越界"],
    order_id: Annotated[str, "订单号，形如 A1234；工单里没有就填空字符串"],
) -> dict:
    """提交一张工单的分类结果。只收结果，不退款、不改地址。"""
    return {}
# ANCHOR_END: tool


TOOL_PROMPT = BASE + "请调用 submit_ticket 提交分类结果；order_id 没有就填空字符串。"


def ask(system: str, text: str, **kw):
    return client.chat.completions.create(
        model=MODEL,
        messages=[{"role": "system", "content": system}, {"role": "user", "content": text}],
        max_tokens=600,
        extra_body={"thinking": {"type": "disabled"}},
        **kw,
    )


def run_free(t) -> dict:
    resp = ask(FREE, t["text"])
    raw = resp.choices[0].message.content or ""
    m = re.search(r"类别[:：]\s*([^\s，。；,;、]+)", raw)
    cat = m.group(1).strip("。，,.;；") if m else None
    return {"cat": cat, "parse": cat is not None, "order": bool(re.search(r"A\d{4}", raw)),
            "field": cat is not None and (bool(re.search(r"A\d{4}", raw))
                                         or not re.search(r"A\d{4}", t["text"])),
            "usage": resp.usage, "raw": raw}


def run_json(t) -> dict:
    resp = ask(JSON_PROMPT, t["text"], response_format={"type": "json_object"})
    content = resp.choices[0].message.content or ""
    try:
        obj = json.loads(content) if content.strip() else None
    except json.JSONDecodeError:
        obj = None
    cat = obj.get("category") if isinstance(obj, dict) else None
    expect_order = bool(re.search(r"A\d{4}", t["text"]))
    order = obj.get("order_id") if isinstance(obj, dict) else None
    return {"cat": cat, "parse": isinstance(obj, dict), "order": order not in (None, ""),
            "field": isinstance(obj, dict) and "category" in obj and (not expect_order or order not in (None, "")),
            "usage": resp.usage}


def run_tool(t) -> dict:
    resp = ask(TOOL_PROMPT, t["text"], tools=[submit_ticket.schema()],
               tool_choice={"type": "function", "function": {"name": "submit_ticket"}})
    msg = resp.choices[0].message
    if not msg.tool_calls:
        return {"cat": None, "parse": False, "order": False, "field": False, "usage": resp.usage}
    try:
        args = json.loads(msg.tool_calls[0].function.arguments)
    except json.JSONDecodeError:
        args = {}
    cat = args.get("category")
    expect_order = bool(re.search(r"A\d{4}", t["text"]))
    required = {"category", "order_id"}
    return {"cat": cat, "parse": bool(args), "order": args.get("order_id") not in (None, ""),
            "field": required <= set(args) and (not expect_order or args.get("order_id") not in (None, "")),
            "usage": resp.usage}


# ANCHOR: table
def main() -> None:
    by_id = {t["id"]: t for t in tickets()}
    todo = [by_id[i] for i in IDS]
    methods = [("自由文本+正则", run_free), ("json_object", run_json), ("强制工具", run_tool)]
    print(f"{len(todo)} 张工单 × {len(methods)} 种做法（关闭思考）")
    print(f"{'做法':<14}{'可解析':>6}{'类别一致':>8}{'字段齐全':>8}{'平均 completion':>16}")
    total_calls = 0
    for name, fn in methods:
        rows = [fn(t) for t in todo]
        total_calls += len(rows)
        ok_parse = sum(r["parse"] for r in rows)
        ok_cat = sum(r["parse"] and r["cat"] == t["category"] for r, t in zip(rows, todo))
        ok_field = sum(r["field"] for r in rows)
        avg = sum((r["usage"].completion_tokens or 0) for r in rows) / len(rows)
        print(f"{name:<14}{ok_parse:>4}/{len(rows):<3}{ok_cat:>5}/{len(rows):<3}"
              f"{ok_field:>5}/{len(rows):<3}{avg:>10.0f}")
        if name.startswith("自由"):
            failed = [t["id"] for r, t in zip(rows, todo) if not r["parse"]]
            print(f"  正则没解析出 {len(failed)} 条：" + "、".join(failed))
            first = next((r for r in rows if not r["parse"]), None)
            if first:
                print(f"  例：{' '.join(first['raw'].split())}")
    print(f"共 {total_calls} 次调用")
# ANCHOR_END: table


if __name__ == "__main__":
    main()
