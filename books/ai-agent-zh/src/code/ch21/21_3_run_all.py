# 21.3 全量跑 30 个工单：每个工单独立 Store，max_turns=6，结果写 JSONL。
# mode: run
# timeout: 1800
# expect: 终态通过
# expect: max_turns
import json
from pathlib import Path

from shop import tickets
from lib.eval import by_category, run_suite, totals

RECORDS = run_suite(tickets())
OUT = Path(__file__).resolve().parent / "21_3_run_all.jsonl"


def main() -> None:
    for r in RECORDS:
        mark = "PASS" if r["check"]["ok"] else "FAIL"
        u = r["usage"]
        print(f"{r['ticket_id']} {r['category']:4} {r['turns']} 轮 {len(r['steps'])} 工具 "
              f"{mark:4} {u.get('total_tokens', 0):5} token")
    t = totals(RECORDS)
    with OUT.open("w", encoding="utf-8") as fh:
        for r in RECORDS:
            fh.write(json.dumps(r, ensure_ascii=False) + "\n")

    print()
    print(f"终态通过 {t['state']}/{t['n']}，关键词通过 {t['keyword']}/{t['n']}")
    print(f"平均 {t['turns']:.1f} 轮，{t['calls']:.1f} 次工具，{t['tokens']:.0f} token/单")
    prompt = sum(r["usage"].get("prompt_tokens", 0) for r in RECORDS)
    completion = sum(r["usage"].get("completion_tokens", 0) for r in RECORDS)
    print(f"合计 {prompt + completion:,} token（输入 {prompt:,} / 输出 {completion:,}）")
    print(f"max_turns {t['max_turns']} 次，工具错误 {t['errors']} 次")
    print("按类别：" + "，".join(f"{c} {p}/{n}" for c, p, n in by_category(RECORDS)))
    print(f"写 {len(RECORDS)} 条到 ch21/21_3_run_all.jsonl")


if __name__ == "__main__":
    main()
