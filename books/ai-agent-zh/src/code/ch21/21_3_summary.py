# 21.3 按类别汇总 21_3_run_all.jsonl，只输出 CSV 给图用。
# mode: run
# llm: none
# expect: 退货退款
from collections import Counter
from pathlib import Path

from lib.eval import load_jsonl

HERE = Path(__file__).resolve().parent
ORDER = ["物流查询", "退货退款", "修改地址", "政策咨询", "投诉建议", "其他", "越界"]

# 没有 JSONL 时的内置样例：和 21.3 那次运行一致
FALLBACK = [("物流查询", 1.00, 1.00), ("退货退款", 0.89, 0.78), ("修改地址", 1.00, 0.50),
            ("政策咨询", 1.00, 0.83), ("投诉建议", 1.00, 1.00), ("其他", 1.00, 1.00),
            ("越界", 1.00, 1.00)]


def main() -> None:
    records = load_jsonl(str(HERE / "21_3_run_all.jsonl"))
    print("类别,终态,整体")
    if not records:
        for cat, state, overall in FALLBACK:
            print(f"{cat},{state:.2f},{overall:.2f}")
        return

    total = Counter(r["category"] for r in records)
    state = Counter(r["category"] for r in records if r["check"]["state_ok"])
    overall = Counter(r["category"] for r in records if r["check"]["ok"])
    for cat in ORDER:
        if total[cat]:
            print(f"{cat},{state[cat] / total[cat]:.2f},{overall[cat] / total[cat]:.2f}")


if __name__ == "__main__":
    main()
