# 18.5 1/3/5 个子 agent 的对照：覆盖几条原因、同一查询重复几次、花多少 token。
# mode: run
# timeout: 900
# expect: 3 个
import os
from concurrent.futures import ThreadPoolExecutor
from typing import Literal

from openai import OpenAI
from pydantic import BaseModel, Field

from ch18.lib.common import parse_json, store

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")
NO_THINK = {"thinking": {"type": "disabled"}}

QUESTIONS = [
    "2026 年 Q3 退货率为什么上升，Q4 为什么回落？",
    "退货率从 Q2 的 5.4% 升到 Q3 的 18.3%，主要是什么造成的？",
    "青川出版社那批书和迅达快递，各让 Q3 多出多少退货？",
    "退货期从 7 天改成 15 天，能解释 Q3 的上升吗？Q4 为什么降了？",
    "用数据说明 Q3 退货率上升的原因，并给 Q4 的对照。",
]


def rows(sql, *args):
    return store.query(sql, *args)


def pct(a, b):
    return f"{a / b * 100:.1f}%"


# ---- 固定的证据切片（真实数据，查询一次取好；18.5 只比较拆分方式）----------------
rates = rows("""SELECT CASE WHEN created < '2026-04' THEN 'Q1' WHEN created < '2026-07' THEN 'Q2'
 WHEN created < '2026-10' THEN 'Q3' ELSE 'Q4' END q, COUNT(*) n,
 SUM(EXISTS(SELECT 1 FROM returns r WHERE r.order_id = o.id)) k
 FROM orders o WHERE status <> '已取消' GROUP BY q ORDER BY q""")
rmap = {r["q"]: r for r in rates}
E = {}
E["E1"] = ("季度退货率（returns/orders，排除已取消）："
           + "、".join(f"{q} {pct(rmap[q]['k'], rmap[q]['n'])}（{rmap[q]['k']}/{rmap[q]['n']}）"
                      for q in ("Q1", "Q2", "Q3", "Q4")) + "。")

reasons = rows("""SELECT r.reason, COUNT(*) n FROM returns r JOIN orders o ON o.id = r.order_id
 WHERE o.created >= '2026-07-01' AND o.created < '2026-10-01' GROUP BY r.reason ORDER BY n DESC""")
E["E2"] = "Q3 退货原因（returns.reason）：" + "、".join(f"{r['reason']} {r['n']} 笔" for r in reasons) + "。"

pubs = rows("""SELECT b.publisher, COUNT(*) n FROM returns r JOIN orders o ON o.id = r.order_id
 JOIN books b ON b.id = r.book_id WHERE o.created >= '2026-07-01' AND o.created < '2026-10-01'
 GROUP BY b.publisher ORDER BY n DESC""")
batch = rows("""SELECT COUNT(*) n FROM returns r JOIN orders o ON o.id = r.order_id
 WHERE o.created >= '2026-07-01' AND o.created < '2026-10-01'
 AND r.book_id IN ('B07', 'B12', 'B19') AND r.reason IN ('缺页', '装订脱落')""")[0]["n"]
E["E3"] = ("Q3 按出版社（returns JOIN books）：" + "、".join(f"{r['publisher']} {r['n']} 笔" for r in pubs)
           + f"；B07/B12/B19 的缺页+装订脱落共 {batch} 笔。")

dmg = {r["courier"]: (r["k"], r["n"]) for r in rows("""SELECT courier, SUM(damaged) k, COUNT(*) n
 FROM shipments GROUP BY courier""")}
trans = {r["q"]: r["n"] for r in rows("""SELECT CASE WHEN o.created < '2026-10-01' THEN 'Q3' ELSE 'Q4' END q,
 COUNT(*) n FROM returns r JOIN orders o ON o.id = r.order_id
 WHERE r.reason = '运输破损' AND o.created >= '2026-07-01' GROUP BY q""")}
E["E4"] = (f"破损率（shipments.damaged，2026 年至今，按快递）：迅达 {dmg['迅达快递'][0]}/{dmg['迅达快递'][1]}="
           f"{pct(*dmg['迅达快递'])}，顺路 {dmg['顺路快递'][0]}/{dmg['顺路快递'][1]}={pct(*dmg['顺路快递'])}；"
           f"运输破损退货 Q3 {trans.get('Q3', 0)} 笔、Q4 {trans.get('Q4', 0)} 笔。")

n_b07 = rows("SELECT COUNT(*) n FROM reviews WHERE book_id = 'B07'")[0]["n"]
n_b19 = rows("SELECT COUNT(*) n FROM reviews WHERE book_id = 'B19'")[0]["n"]
n_b12 = rows("SELECT COUNT(*) n FROM reviews WHERE book_id = 'B12'")[0]["n"]
b12_ret = rows("""SELECT COUNT(*) n FROM returns r JOIN orders o ON o.id = r.order_id
 WHERE r.book_id = 'B12' AND o.created >= '2026-07-01' AND o.created < '2026-10-01'""")[0]["n"]
E["E5"] = (f"B07 评论 {n_b07} 条，1 星：“缺页！这个批次是不是有问题？”（2026-07-10）；"
           "2 星：“快递太粗暴，书脊都裂了。”（2026-10-22）。")
E["E6"] = f"B19 评论 {n_b19} 条，1 星：“第 120 页到 150 页整个缺了，退了。”（2026-09-20）。"
E["E7"] = (f"B12 评论只有 {n_b12} 条（3 星“内容偏浅”、4 星“物流很快，包装完好”），没有差评；"
           f"B12 的 Q3 退货有 {b12_ret} 笔。")
E["E8"] = "returns.md 退货期限：2026-07-01 起从 7 天改为 15 天（修订记录）。"
E["E9"] = "returns.md 质量问题：缺页、装订脱落、运输破损算质量问题，签收后 60 天内退全款。"
E["E10"] = "shipping.md 快递公司：2026-08-01 起广东、广西、海南改用迅达快递。"

# ANCHOR: configs
# 每种配置怎么切：每条线 = (名字, 查了哪些数据, 拿到哪些证据编号)
CONFIGS = {
    1: [("general", ["sql:quarterly", "sql:reasons", "sql:publishers", "sql:courier",
                     "reviews:B07", "reviews:B12", "reviews:B19", "policy:returns", "policy:shipping"],
         ["E1", "E2", "E3", "E4", "E5", "E6", "E7", "E8", "E9", "E10"])],
    3: [("sql", ["sql:quarterly", "sql:reasons", "sql:publishers", "sql:courier"], ["E1", "E2", "E3", "E4"]),
        ("reviews", ["reviews:B07", "reviews:B12", "reviews:B19"], ["E5", "E6", "E7"]),
        ("policy", ["policy:returns", "policy:shipping"], ["E8", "E9", "E10"])],
    5: [("sql-reason", ["sql:quarterly", "sql:reasons", "sql:publishers"], ["E1", "E2", "E3"]),
        ("sql-ship", ["sql:quarterly", "sql:courier"], ["E1", "E4"]),
        ("reviews-ab", ["reviews:B07", "reviews:B19"], ["E5", "E6"]),
        ("reviews-b12", ["reviews:B12"], ["E7"]),
        ("policy", ["policy:returns", "policy:shipping"], ["E8", "E9", "E10"])],
}
# ANCHOR_END: configs

Cause = Literal["批次", "快递", "政策", "其他"]
SYSTEM = """你是调研子 agent。只根据给出的证据回答，不要补充证据外的事实。
只输出 JSON：{"findings": [{"cause": "批次|快递|政策|其他", "claim": "一句结论，必须带数字",
 "evidence": "证据编号和数字", "source": "表名或文档名"}], "gaps": []}。"""


class F(BaseModel):
    cause: Cause
    claim: str
    evidence: str
    source: str


class Out(BaseModel):
    findings: list[F] = Field(default_factory=list)
    gaps: list[str] = Field(default_factory=list)


def ask(question: str, pack: list[str], limit: int) -> tuple[Out, int]:
    text = "\n".join(f"[{i}] {E[i]}" for i in pack)
    user = f"问题：{question}\n\n证据：\n{text}\n\n最多写 {limit} 条 finding。"
    total = 0
    for attempt in (1, 2):
        resp = client.chat.completions.create(
            model=MODEL,
            messages=[{"role": "system", "content": SYSTEM}, {"role": "user", "content": user}],
            response_format={"type": "json_object"}, max_tokens=700, extra_body=NO_THINK)
        total += resp.usage.prompt_tokens + resp.usage.completion_tokens
        try:
            return Out.model_validate(parse_json(resp.choices[0].message.content or "")), total
        except Exception:                                   # noqa: BLE001 - 重试一次
            user += "\n上一次不是合法 JSON，按格式重写。"
    return Out(), total


# ANCHOR: run
def run_config(question: str, n: int) -> tuple[set[str], int, int, list[Out]]:
    teams = CONFIGS[n]
    # 1 条线时要能写出三条原因，上限给 3；3/5 条线每条线最多 2 条。
    limit = 3 if n == 1 else 2
    with ThreadPoolExecutor(max_workers=n) as pool:
        futures = [pool.submit(ask, question, pack, limit) for _, _, pack in teams]
        outs = [f.result()[0] for f in futures]
        tokens = sum(f.result()[1] for f in futures)
    causes = {f.cause for out in outs for f in out.findings
              if f.cause != "其他" and any(c.isdigit() for c in f.claim + f.evidence)}
    lookups = [x for _, ls, _ in teams for x in ls]
    dup = len(lookups) - len(set(lookups))
    return causes, dup, tokens, outs
# ANCHOR_END: run


def main() -> None:
    print("5 个问题 × 3 种配置 = 15 次调研；每个子 agent 只做一次判断（证据已按切片取好）\n")
    stats = {n: {"cover": [], "dup": [], "tok": []} for n in (1, 3, 5)}
    by_q = []
    for q in QUESTIONS:
        line = []
        for n in (1, 3, 5):
            causes, dup, tokens, _ = run_config(q, n)
            stats[n]["cover"].append(len(causes))
            stats[n]["dup"].append(dup)
            stats[n]["tok"].append(tokens)
            line.append(len(causes))
        by_q.append(line)

    print("配置     平均覆盖/3   重复查询/次   平均 token")
    for n in (1, 3, 5):
        s = stats[n]
        print(f"{n} 个     {sum(s['cover']) / 5:>4.1f}        {sum(s['dup']) / 5:>4.1f}       "
              f"{sum(s['tok']) // 5:>6,}")
    print("\n每个问题的覆盖（1/3/5 个）：" + "  ".join(
        f"Q{i + 1} {'/'.join(map(str, x))}" for i, x in enumerate(by_q)))
    print("\n覆盖口径：三条原因（6 月批次、快递破损、退货期）里，claim 带数字的算一条；只数原因，不校验数字对不对。")


if __name__ == "__main__":
    main()
