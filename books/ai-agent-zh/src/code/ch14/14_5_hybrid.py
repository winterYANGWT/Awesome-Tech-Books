# 14.5 混合策略：退货期限用预取摘要，广州快递查政策，9 月破损写 SQL。
# mode: run
# expect: 迅达快递
import os

from harness import ToolError, Toolbox, run, tool
from shop import ShopError, Store
from shop.tools import SUPPORT_PROMPT

MODEL = os.environ.get("MODEL", "deepseek-flash")
store = Store()

# 固定上下文：每次都相关、稳定、很短，带来源和生效日期
# ANCHOR: fixed
FIXED = SUPPORT_PROMPT + """

固定资料（依据 returns.md，2026-07-01 起）：
- 退货期限：签收后 15 天内可以申请退货，以物流签收日期为准。
- 质量问题：缺页、错装、印刷模糊、装订脱落、运输中破损都算；退全款，签收后 60 天内可申请。"""
# ANCHOR_END: fixed


@tool
def search_policies(query: str) -> list[dict]:
    """在政策文档里查相关条款，返回最相关的 3 节。"""
    hits = store.search_policies(query)
    if not hits:
        raise ToolError(f"没有找到；可用文档：{', '.join(store.policy_names())}")
    return [{"section": h["title"], "text": h["text"]} for h in hits]


@tool
def read_section(doc: str, title: str) -> dict:
    """读一节正文；title 用完整标题，例如“配送 / 快递公司”。"""
    sections = [s for s in store.policy_sections() if s["doc"] == doc]
    match = next((s for s in sections if s["title"] == title), None)
    if match is None:
        raise ToolError(f"{doc} 里没有“{title}”")
    return {"doc": doc, "section": match["title"], "text": match["text"], "length": len(match["text"])}


@tool
def list_tables() -> list[dict]:
    """列出数据库里的表和行数。"""
    names = ["orders", "shipments", "order_items", "books", "customers", "returns", "reviews"]
    return [{"table": t, "rows": store.query(f"SELECT COUNT(*) n FROM {t}")[0]["n"]} for t in names]


@tool
def sql_query(sql: str, max_rows: int = 50) -> dict:
    """执行一条只读 SQL（只能 SELECT）；最多返回 max_rows 行。"""
    try:
        rows = store.readonly_sql(sql, max_rows + 1)
    except ShopError as e:
        raise ToolError(f"{e}。只允许 SELECT") from None
    truncated = len(rows) > max_rows
    rows = rows[:max_rows]
    out = {"columns": list(rows[0]) if rows else [], "rows": rows, "returned": len(rows),
           "truncated": truncated}
    if truncated:
        out["hint"] = "还有更多行：用 WHERE、ORDER BY 或 LIMIT/OFFSET 缩小范围"
    return out


BOX = Toolbox([search_policies, read_section, list_tables, sql_query])
PROMPT = """你是青石书店的客服。固定资料里已经有的内容直接回答，不要调用工具。
其他问题：政策用 search_policies，数据用 list_tables + sql_query。每次最多 4 轮；回答只给事实和数字，不给建议，不超过 80 字。"""

QUESTIONS = [
    "签收以后多少天内可以退货？",
    "寄到广州用的是哪家快递？能换顺路吗？",
    "2026 年 9 月，迅达快递和顺路快递各有多少笔破损？",
]


def main() -> None:
    messages = [{"role": "system", "content": FIXED + "\n\n" + PROMPT}]
    for i, q in enumerate(QUESTIONS, 1):
        messages.append({"role": "user", "content": q})
        res = run(messages, BOX, model=MODEL, max_turns=4, verbose=False)
        names = [s.name for s in res.steps]
        answer = " ".join((res.answer or "").split())[:200]
        print(f"第 {i} 问：{q}")
        print(f"  回答：{answer}")
        print(f"  工具 {len(res.steps)} 次：{'、'.join(names) or '—'}；"
              f"输入 {res.usage.get('prompt_tokens', 0)} token")


if __name__ == "__main__":
    main()
