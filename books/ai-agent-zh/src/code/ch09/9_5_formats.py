# 9.5 同一张订单，JSON、Markdown 表、key=value 三种返回格式的 token 和正确率。
# mode: run
# output: varies
# expect: JSON
# expect: Markdown
# expect: compact
# timeout: 600
"""T01、T06 × 3 种格式 × 5 次：工具内容是同一组字段，只换排版。"""
import json

from harness import Toolbox, run, tool

from shop import Store, TODAY, tickets

NO_THINK = {"thinking": {"type": "disabled"}}
TICKETS = {t["id"]: t for t in tickets()}
SYSTEM = """你是青石书店的客服。先用 get_order 查订单，再回答；只根据工具结果回答，不猜。
回答简洁，用中文。"""


# ANCHOR: render
def flat(order: dict) -> dict:
    s = order["shipment"]
    return {"order_id": order["id"], "status": order["status"], "created": order["created"],
            "books": "、".join(f"{i['title']}×{i['qty']}" for i in order["items"]),
            "total_paid": order["total"], "refunded": order["refunded"],
            "shipping_fee": order["shipping_fee"], "courier": s["courier"],
            "delivered": s["delivered"] or "未签收", "latest": s["events"][-1]}


def render(order: dict, fmt: str) -> str:
    d = flat(order)
    if fmt == "JSON":
        return json.dumps(d, ensure_ascii=False)
    if fmt == "Markdown":
        keys = list(d)
        head = "| " + " | ".join(keys) + " |"
        line = "|" + "|".join("---" for _ in keys) + "|"
        row = "| " + " | ".join(str(d[k]) for k in keys) + " |"
        return "\n".join([head, line, row])
    return "；".join(f"{k}={v}" for k, v in d.items())
# ANCHOR_END: render


def make_box(store: Store, fmt: str) -> Toolbox:
    @tool
    def get_order(order_id: str) -> str:
        """按订单号查订单：状态、书目、金额、物流。"""
        o = store.order(order_id)
        return render(o, fmt) if o else "没有这张订单"

    return Toolbox([get_order])


def once(tid: str, fmt: str) -> dict:
    ticket = TICKETS[tid]
    res = run([{"role": "system", "content": SYSTEM}, {"role": "user", "content": ticket["text"]}],
              make_box(Store(), fmt), max_turns=4, verbose=False,
              extra_body=NO_THINK, max_tokens=400)
    answer = res.answer or ""
    want = "迅达快递" if tid == "T01" else "82"
    return {"ok": want in answer, "prompt": res.usage.get("prompt_tokens", 0), "turns": res.turns}


# ANCHOR: main
def main() -> None:
    o = Store().order("A1670")
    sizes = "，".join(f"{fmt} {len(render(o, fmt))} 字符" for fmt in ("JSON", "Markdown", "compact"))
    print(f"同一张 A1670，三种排版的长度：{sizes}")
    print("格式      任务  正确   平均输入 token  平均轮数")
    for fmt in ("JSON", "Markdown", "compact"):
        for tid in ("T01", "T06"):
            rows = [once(tid, fmt) for _ in range(5)]
            n = len(rows)
            avg = lambda k: sum(r[k] for r in rows) / n
            print(f"{fmt:<9} {tid}   {sum(r['ok'] for r in rows)}/{n}    "
                  f"{avg('prompt'):.0f}            {avg('turns'):.1f}")
    print("检查点：T01 要说出迅达快递，T06 要算出 82 元（76 元书款 + 6 元运费）。")


if __name__ == "__main__":
    main()
# ANCHOR_END: main
