# mode: fail
# llm: none
# expect: 缺片
# expect: B07-R07
"""模拟 map 少了一片，reduce 却按“10 片都到齐”去索引：KeyError。"""
from collections import Counter

# 正常应该收到 B07-R01 到 B07-R10；这里故意少了 R07。
PARTS = [{"review_id": f"B07-R{i:02d}", "issue": "无问题", "stars": 5, "quote": "好"}
         for i in range(1, 11) if i != 7]
EXPECTED = [f"B07-R{i:02d}" for i in range(1, 11)]

print(f"收到 {len(PARTS)} 片，准备按 {len(EXPECTED)} 片汇总")
counts = Counter()
by_id = {p["review_id"]: p for p in PARTS}

for rid in EXPECTED:
    try:
        p = by_id[rid]          # R07 不在，抛 KeyError
    except KeyError:
        raise KeyError(f"缺片：{rid}；已收到 {len(PARTS)}/{len(EXPECTED)} 片") from None
    counts[p["issue"]] += 1

print("汇总：", dict(counts))
