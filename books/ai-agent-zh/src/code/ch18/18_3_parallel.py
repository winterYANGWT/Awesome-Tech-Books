# 18.3 三条调研线并行：SQL 查数量，评论查原话，政策查条款和生效日期。
# mode: run
# timeout: 150
# expect: 18.3%
import json
import os
import sys
from concurrent.futures import ThreadPoolExecutor

from openai import OpenAI

from ch18.lib.common import (POLICY_BOX, REVIEW_BOX, RESEARCH_PROMPT, SQL_BOX, parse_result,
                             store)
from harness import run, short

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")

QUESTION = "2026 年 Q3 退货率为什么上升，Q4 为什么回落？"

# ANCHOR: tasks
TASKS = {
    "sql": """你只查退货记录，不查评论和政策。问题：Q3 退货率为什么上升，Q4 为什么回落？
按顺序原样执行下面 4 条只读 SQL，不要改 WHERE 和 GROUP BY，把返回的数字抄进 findings：
1) SELECT CASE WHEN o.created < '2026-04-01' THEN 'Q1' WHEN o.created < '2026-07-01' THEN 'Q2'
   WHEN o.created < '2026-10-01' THEN 'Q3' ELSE 'Q4' END AS q, COUNT(*) AS orders,
   SUM(EXISTS(SELECT 1 FROM returns r WHERE r.order_id = o.id)) AS returned
   FROM orders o WHERE o.status <> '已取消' GROUP BY q ORDER BY q
2) SELECT CASE WHEN o.created < '2026-10-01' THEN 'Q3' ELSE 'Q4' END AS q, r.reason, COUNT(*) AS n
   FROM returns r JOIN orders o ON o.id = r.order_id
   WHERE o.created >= '2026-07-01' GROUP BY q, r.reason ORDER BY q, n DESC
3) SELECT b.publisher, COUNT(*) AS n,
   SUM(r.book_id IN ('B07', 'B12', 'B19') AND r.reason IN ('缺页', '装订脱落')) AS batch
   FROM returns r JOIN orders o ON o.id = r.order_id JOIN books b ON b.id = r.book_id
   WHERE o.created >= '2026-07-01' AND o.created < '2026-10-01' GROUP BY b.publisher ORDER BY n DESC
4) SELECT courier, SUM(damaged) AS damaged, COUNT(*) AS n FROM shipments GROUP BY courier
findings 恰好 4 条，每条对应一项，数字从查询结果原样抄：
1) Q1–Q4 退货率：Q1 5.6%（10/178）、Q2 5.4%（10/185）、Q3 18.3%（38/208）、Q4 8.2%（8/98）；
2) Q3 缺页 15 笔、装订脱落 5 笔，合计 20 笔；
3) 运输破损 Q3 9 笔、Q4 8 笔，合计 17 笔；
4) 迅达 16/65=24.6%，顺路 6/601=1.0%。
如果查询结果和上面的数字对不上，以查询结果为准，并在 gaps 里写清楚。每条 finding 写 claim、evidence、source。最后只输出 JSON，status 只能是 ok 或 failed。""",
    "reviews": """你只读评论，不查 SQL 和政策。问题：Q3 退货率为什么上升？
分别读 B07、B12、B19 的评论：各有多少条、差评（只算 1–2 星）有几条、差评原话是什么、哪本书的评论样本不足以支撑结论。
评论只能佐证，不能替代退货记录。至少给 3 条 finding。""",
    "policy": """你只查政策文档，不查 SQL 和评论。问题：退货期和快递在 2026 年改过什么？
findings 至少 3 条，每条写文档名、小节和生效日期：
1) returns.md：2026-07-01 起退货期限从 7 天改成 15 天；
2) returns.md：质量问题签收后 60 天内退全款；
3) shipping.md：2026-08-01 起广东、广西、海南改用迅达快递。
第 3 条必须调用 read_section(doc="shipping", title="配送 / 快递公司") 读到原文，不能只查退货政策。""",
}
# ANCHOR_END: tasks


def researcher(name: str, task: str, box):
    messages = [{"role": "system", "content": RESEARCH_PROMPT},
                {"role": "user", "content": f"任务（{name} 线）：\n{task}"}]
    res = None
    for attempt in (1, 2):
        res = run(messages, box, max_turns=5, verbose=False)
        try:
            return parse_result(res.answer or ""), res
        except ValueError as e:
            print(f"  {name} 第 {attempt} 次 JSON 不合格：{e}", file=sys.stderr)
            print(f"  原始回答：{(res.answer or '<空>')[:300]}", file=sys.stderr)
            fix = client.chat.completions.create(
                model=MODEL, messages=messages + [{"role": "user",
                          "content": "上一次不是合法 JSON。不要再调用工具，按合同直接输出 JSON 对象。"}],
                response_format={"type": "json_object"}, max_tokens=1200)
            try:
                return parse_result(fix.choices[0].message.content or ""), res
            except ValueError as e2:
                print(f"  {name} 修复调用仍不合格：{e2}", file=sys.stderr)
    from ch18.lib.common import ResearchResult
    return ResearchResult(findings=[], status="failed", gaps=["子 agent 没有返回合法 JSON"]), res


def main() -> None:
    # ANCHOR: parallel
    with ThreadPoolExecutor(max_workers=3) as pool:
        futures = {name: pool.submit(researcher, name, TASKS[name], box)
                   for name, box in (("sql", SQL_BOX), ("reviews", REVIEW_BOX), ("policy", POLICY_BOX))}
        results = {name: futures[name].result() for name in ("sql", "reviews", "policy")}
    # ANCHOR_END: parallel

    tokens = 0
    summaries = []
    print("三个子 agent 同时发出，按固定顺序取回：")
    for name, (result, res) in results.items():
        tokens += res.usage.get("prompt_tokens", 0) + res.usage.get("completion_tokens", 0)
        print(f"\n{name} 子 agent：status={result.status} turns={res.turns} "
              f"findings={len(result.findings)} gaps={len(result.gaps)}")
        for f in result.findings[:4]:
            print(f"  - {short(f.claim, 80)}  [{short(f.source, 40)}]")
        for g in result.gaps[:2]:
            print(f"  gap：{short(g, 80)}")
        summaries += [f"[{name}] {f.claim}（{f.evidence}；{f.source}）" for f in result.findings]

    print(f"\n子 agent 合计 {tokens} token")
    # ANCHOR: synthesize
    prompt = "下面是三条调研线的 findings。用 3 条要点回答问题，每条不超过 45 字，带一个数字和来源。" \
             "不要新增 findings 之外的事实。\n\n" + "\n".join(summaries)
    resp = client.chat.completions.create(
        model=MODEL, messages=[{"role": "user", "content": prompt}], max_tokens=600, extra_body={"thinking": {"type": "disabled"}})
    print(f"主 agent 的合成 prompt：{resp.usage.prompt_tokens} token")
    print(resp.choices[0].message.content)
    # ANCHOR_END: synthesize
    first = results["sql"][0].findings
    print(f"\nfindings 的 JSON 示例：" + (json.dumps(first[0].model_dump(), ensure_ascii=False)[:110] + "…" if first else "（SQL 线没有 finding）"))


if __name__ == "__main__":
    main()
