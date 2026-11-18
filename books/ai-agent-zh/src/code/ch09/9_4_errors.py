# 9.4 同一个“已发货不能改地址”，两种错误文字：invalid，和带原因的恢复指南。
# mode: run
# output: varies
# expect: 已发货
# expect: 转寄
# timeout: 600
"""T17、T18 × 2 种错误文本 × 6 次：看模型是否重复调用、是否给出替代方案。"""
from harness import ToolError, Toolbox, run, tool

from shop import TODAY

NO_THINK = {"thinking": {"type": "disabled"}}
SYSTEM = f"""你是青石书店的客服。今天是 {TODAY.isoformat()}。
顾客要求改地址时，直接调用 update_address 办理，不需要先查订单；然后根据工具返回写回复。
回答简洁，用中文，直接告诉顾客结果和下一步。"""
CASES = {
    "T17": ("订单 A1676 能改到公司地址吗？新地址：南京市鼓楼区中山北路 12 号。", "A1676"),
    "T18": ("订单 A1673 能改到公司地址吗？新地址：苏州市工业园区星湖街 88 号。", "A1673"),
}
GOOD = ("订单 {oid} 状态是“已发货”，只有待发货的订单能改地址；"
        "可以联系快递转寄到新地址，或让顾客拒收后重新下单。")


def make_box(oid: str, helpful: bool) -> Toolbox:
    @tool(risky=True)
    def update_address(order_id: str, address: str) -> dict:
        """修改订单的收货地址。只有“待发货”的订单能改。"""
        if helpful:
            raise ToolError(GOOD.format(oid=oid))
        raise ToolError("invalid")

    return Toolbox([update_address])


def once(text: str, oid: str, helpful: bool) -> dict:
    res = run([{"role": "system", "content": SYSTEM}, {"role": "user", "content": text}],
              make_box(oid, helpful), max_turns=6, verbose=False,
              extra_body=NO_THINK, max_tokens=400)
    answer = res.answer or ""
    shipped = any(w in answer for w in ("已发货", "已经发货", "发货了"))
    return {"calls": sum(s.name == "update_address" for s in res.steps), "turns": res.turns,
            "shipped": shipped, "alt": ("转寄" in answer or "拒收" in answer)}


# ANCHOR: main
def main() -> None:
    print("工单  错误文本        调用次数  轮数  提到已发货  提到转寄/拒收")
    for tid, (text, oid) in CASES.items():
        for label, helpful in (("无信息", False), ("可执行", True)):
            rows = [once(text, oid, helpful) for _ in range(6)]
            n = len(rows)
            avg = lambda k: sum(r[k] for r in rows) / n
            print(f"{tid}  {label:<6}        {avg('calls'):.1f}      {avg('turns'):.1f}   "
                  f"{sum(r['shipped'] for r in rows)}/{n}        "
                  f"{sum(r['alt'] for r in rows)}/{n}")
    print("说明：两组只换错误文本；“无信息”组拿到的是“错误：invalid”，不知道错在哪。")


if __name__ == "__main__":
    main()
# ANCHOR_END: main
