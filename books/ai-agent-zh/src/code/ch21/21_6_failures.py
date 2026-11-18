# 21.6 按 check.kind 给失败分组，每类看前两条；不调用模型。
# mode: run
# llm: none
# expect: 失败类型
from collections import Counter, defaultdict
from pathlib import Path

from lib.eval import load_jsonl

HERE = Path(__file__).resolve().parent
FALLBACK = [
    {"ticket_id": "T14", "category": "退货退款", "turns": 3,
     "check": {"ok": False, "kind": "政策误读", "reason": "不该退款却退了 A1649", "state_ok": False}},
    {"ticket_id": "T12", "category": "退货退款", "turns": 2,
     "check": {"ok": False, "kind": "关键词缺", "reason": "没提到 15 天", "state_ok": True}},
    {"ticket_id": "T17", "category": "修改地址", "turns": 2,
     "check": {"ok": False, "kind": "关键词缺", "reason": "没提到 已发货", "state_ok": True}},
    {"ticket_id": "T18", "category": "修改地址", "turns": 2,
     "check": {"ok": False, "kind": "关键词缺", "reason": "没提到 已发货", "state_ok": True}},
    {"ticket_id": "T24", "category": "政策咨询", "turns": 1,
     "check": {"ok": False, "kind": "关键词缺", "reason": "没提到 迅达快递", "state_ok": True}},
]


def main() -> None:
    records = load_jsonl(str(HERE / "21_3_run_all.jsonl")) or FALLBACK
    failed = [r for r in records if not r["check"]["ok"]]
    kinds = Counter(r["check"]["kind"] for r in failed)
    state_bad = sum(not r["check"]["state_ok"] for r in failed)
    print(f"失败类型 {len(failed)} 条（终态错 {state_bad}、关键词错 {len(failed) - state_bad}）："
          + "、".join(f"{k} {n}" for k, n in kinds.most_common()))
    groups = defaultdict(list)
    for r in failed:
        groups[r["check"]["kind"]].append(r)
    for kind, n in kinds.most_common():
        print(f"\n{kind} {n} 条")
        for r in groups[kind][:2]:
            print(f"  {r['ticket_id']} {r['check']['reason']}（{r['turns']} 轮）")


if __name__ == "__main__":
    main()
