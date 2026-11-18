# 21.5 对照实验：旧提示 vs 新提示，10 个工单各跑 2 次。
# mode: run
# timeout: 1800
# expect: 旧提示
# expect: 新提示
from shop import tickets
from lib.eval import BASE_PROMPT, NEW_PROMPT, run_suite, totals

IDS = ["T01", "T06", "T09", "T12", "T13", "T14", "T15", "T17", "T24", "T27"]
T = {t["id"]: t for t in tickets()}
PICKS = [T[i] for i in IDS]
ROUNDS = 2


def main() -> None:
    rows = []
    for label, prompt in (("旧提示", BASE_PROMPT), ("新提示", NEW_PROMPT)):
        for k in range(1, ROUNDS + 1):
            records = run_suite(PICKS, prompt)
            t = totals(records)
            passed = sum(r["check"]["ok"] for r in records)
            rows.append((label, passed, len(records), t["turns"], t["tokens"]))
            print(f"{label} 第 {k} 次：通过 {passed}/{len(records)}，"
                  f"平均 {t['turns']:.1f} 轮，平均 {t['tokens']:.0f} token")

    print()
    print("| 组 | 通过 | 平均轮数 | 平均 token |")
    for label in ("旧提示", "新提示"):
        group = [r for r in rows if r[0] == label]
        n = sum(r[2] for r in group)
        passed = sum(r[1] for r in group)
        turns = sum(r[3] * r[2] for r in group) / n
        tokens = sum(r[4] * r[2] for r in group) / n
        print(f"| {label} | {passed}/{n} | {turns:.1f} | {tokens:.0f} |")
    print("\n只看方向：两组各 20 次，样本小，不写成精确的提升幅度。")


if __name__ == "__main__":
    main()
