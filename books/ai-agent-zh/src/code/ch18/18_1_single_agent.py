# 18.1 单 agent 带 10 个工具回答同一个开放问题：它查了几条线就收工？
# mode: run
# expect: Q3
import json
import re

from harness import fmt_call, run, short

from ch18.lib.common import readonly_sql, store
from shop import TODAY
from shop.tools import support_tools

QUESTION = "2026 年 Q3 退货率为什么上升，Q4 为什么回落？给出数据依据。"

PROMPT = f"""你是青石书店的运营分析助手。今天是 {TODAY.isoformat()}。
用工具查实际情况，不要凭印象回答。最多调用 5 次工具，回答不超过 180 字。
口径：按 orders.created 分季度；排除 status='已取消' 的订单；有退货指 returns 里有该 order_id。
百分比保留一位小数（例如 18.3%），不要写成 18.27%。
结论后用括号注明依据的表名或政策小节。"""

# 9 个客服工具加一条只读 SQL：单 agent 手里什么都有
box = support_tools(store)
box.add(readonly_sql)


def covers(answer: str) -> dict:
    """一条原因要有自己的关键词和数字，才算带着证据被覆盖。"""
    segs = re.split(r"[。\n；]", answer)
    def has(words: str, num: str) -> bool:
        return any(re.search(words, s) and re.search(num, s) for s in segs)
    return {
        "批次（缺页/装订）": has(r"缺页|装订", r"20|15\s*(?:笔|单)|5\s*(?:笔|单)|34|22"),
        "快递（运输破损）": has(r"运输破损|迅达|快递", r"17|24\.6|16|9\s*(?:笔|单)|8\s*(?:笔|单)"),
        "退货期（7→15 天）": has(r"退货期|15\s*天|7\s*天", r"7|15|2026-07-01"),
    }


def main() -> None:
    print(f"问题：{QUESTION}")
    messages = [{"role": "system", "content": PROMPT}, {"role": "user", "content": QUESTION}]
    res = run(messages, box, max_turns=6, verbose=False)

    for i, s in enumerate(res.steps, 1):
        print(f"  工具 {i}：{short(fmt_call(s.name, s.arguments), 70)}")
    print(f"\n回答：{res.answer}")

    hit = covers(res.answer or "")
    print("\n覆盖检查（原因要有数字才算）")
    for name, ok in hit.items():
        print(f"  {name}: {'✓' if ok else '✗'}")
    print(f"  三条原因覆盖 {sum(hit.values())}/3")
    q = {k: bool(re.search(k, res.answer or "")) for k in ("5.6", "5.4", "18.3", "8.2")}
    print("  季度数字：" + " ".join(f"Q{i + 1} {'✓' if ok else '✗'}" for i, ok in enumerate(q.values())))
    tables = len(re.findall(r"\b(?:returns|orders|books|shipments)\b", res.answer or ""))
    docs = len(re.findall(r"returns\.md|shipping\.md|退货期限|快递公司", res.answer or ""))
    print(f"  引用：表名 {tables} 处，政策小节 {docs} 处")
    print(f"轮数 {res.turns}，工具调用 {len(res.steps)} 次，输入 {res.usage.get('prompt_tokens', 0)} token")
    print(f"工具参数示例：{json.dumps(res.steps[0].arguments, ensure_ascii=False)[:80] if res.steps else '—'}")


if __name__ == "__main__":
    main()
