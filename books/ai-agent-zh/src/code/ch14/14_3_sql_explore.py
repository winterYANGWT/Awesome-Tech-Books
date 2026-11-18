# 14.3 先看表结构，再写一条 JOIN：9 月两家快递各有多少笔破损。
# mode: run
# expect: 迅达快递
import os

from harness import ToolError, Toolbox, run, tool

from shop import ShopError, Store

MODEL = os.environ.get("MODEL", "deepseek-flash")
store = Store()
LAST: dict = {}

USE = {"orders": "订单：顾客、日期、金额、快递、状态", "shipments": "物流：快递公司、发货/签收日期、是否破损",
       "order_items": "订单里的书", "books": "书目", "customers": "顾客", "returns": "退货记录",
       "reviews": "评论", "actions": "agent 做过的操作"}


# ANCHOR: tools
@tool
def list_tables() -> list[dict]:
    """列出数据库里的表和行数。先看这个，再决定查哪张表。"""
    rows = store.query("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name")
    return [{"table": r["name"], "rows": store.query(f"SELECT COUNT(*) n FROM {r['name']}")[0]["n"],
             "use": USE.get(r["name"], "")} for r in rows]


@tool
def describe_table(name: str) -> dict:
    """看一张表的字段和类型。name 必须是 list_tables 返回的表名。"""
    if name not in USE:
        raise ToolError(f"没有表 {name}；可用：{', '.join(sorted(USE))}")
    cols = store.query(f"PRAGMA table_info({name})")
    return {"table": name, "columns": [{"name": c["name"], "type": c["type"],
                                        "pk": bool(c["pk"])} for c in cols]}


@tool
def sql_query(sql: str, max_rows: int = 50) -> dict:
    """执行一条只读 SQL（只能 SELECT），最多返回 max_rows 行。写操作会被数据库拒绝。"""
    try:
        rows = store.readonly_sql(sql, max_rows + 1)   # 多取一行，才知道有没有被截断
    except ShopError as e:
        raise ToolError(f"{e}。只允许 SELECT；写操作请交给业务工具") from None
    truncated = len(rows) > max_rows
    rows = rows[:max_rows]
    result = {"columns": list(rows[0]) if rows else [], "rows": rows, "returned": len(rows),
              "truncated": truncated}
    if truncated:
        result["hint"] = f"还有更多行：用 WHERE、ORDER BY 或 LIMIT/OFFSET 缩小范围"
    LAST.clear()
    LAST.update(result)
    return result
# ANCHOR_END: tools


PROMPT = """你是青石书店的运营数据分析助手。
先 list_tables 看有哪些表，再用 describe_table 看 orders 和 shipments 的字段，然后写一条只读 SQL 回答。
只查这两张表，不要校验数据质量，不要重复查询；最多调用 5 次工具。不要 SELECT *。
回答不超过 80 字：先给两行数字，再给一句结论。"""


def main() -> None:
    question = "2026 年 9 月，迅达快递和顺路快递各有多少笔破损？"
    messages = [{"role": "system", "content": PROMPT}, {"role": "user", "content": question}]
    res = run(messages, Toolbox([list_tables, describe_table, sql_query]), model=MODEL, max_turns=6)
    print(f"\n回答：{res.answer}")
    print(f"轮数 {res.turns}，工具调用 {len(res.steps)} 次，输入 {res.usage.get('prompt_tokens', 0)} token")
    print(f"最后一次 SQL：{LAST.get('returned')} 行，truncated={LAST.get('truncated')}，"
          f"列 {LAST.get('columns')}")


if __name__ == "__main__":
    main()
