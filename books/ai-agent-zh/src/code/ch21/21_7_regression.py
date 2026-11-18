# 21.7 回归对比：候选版本修好了几条、弄坏了哪条。有“弄坏”就非 0 退出。
# mode: fail
# llm: none
# expect: 弄坏
# expect: 修好 3
import sys
from pathlib import Path

from lib.eval import diff_runs, load_jsonl

HERE = Path(__file__).resolve().parent
BASE = load_jsonl(str(HERE / "baseline.jsonl")) or []
CAND = load_jsonl(str(HERE / "candidate.jsonl")) or []


def main() -> None:
    d = diff_runs(BASE, CAND)
    print(f"修好 {len(d['fixed'])}、弄坏 {len(d['broken'])}、仍失败 {len(d['still'])}")
    print("修好：" + "、".join(r["ticket_id"] for r in d["fixed"]))
    for r in d["broken"]:
        print(f"弄坏：{r['ticket_id']} {r['check']['kind']} {r['check']['reason']}")
    print("仍失败：" + "；".join(f"{r['ticket_id']} {r['check']['reason']}" for r in d["still"]))
    if d["broken"]:
        print(f"有回归：{len(d['broken'])} 条从通过变成失败", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
