# 14.4 同样 10 个问题：A 只给 top-3 预检索，B 给目录、读取和 SQL；各跑 2 轮。
# mode: run
# expect: 按需
# timeout: 900
import os
import re

from openai import OpenAI

from harness import ToolError, Toolbox, run, tool
from shop import ShopError, Store

client = OpenAI()                                  # 读 OPENAI_BASE_URL、OPENAI_API_KEY
MODEL = os.environ.get("MODEL", "deepseek-flash")
store = Store()
ROUNDS = 2

TABLES = ["orders", "shipments", "order_items", "books", "customers", "returns", "reviews"]


def has(text: str, *words: str) -> bool:
    flat = re.sub(r"\s+", "", text or "")
    return all(w in flat for w in words)


# 6 个政策问题 + 4 个数据问题；check 判断回答里有没有预期事实
# ANCHOR: questions
QUESTIONS = [
    ("T19", "政策", "积分怎么用？100 积分能抵多少钱？", lambda t: has(t, "1元")),
    ("T20", "政策", "公司报销要增值税专用发票，能开吗？需要提供什么？", lambda t: has(t, "税号")),
    ("T21", "政策", "买 80 块钱的书要运费吗？", lambda t: has(t, "6元")),
    ("T22", "政策", "签收以后多少天内可以退货？", lambda t: has(t, "15天")),
    ("T23", "政策", "我是银卡会员，生日有什么福利？", lambda t: has(t, "20元")),
    ("T24", "政策", "寄到广州用的是哪家快递？能换顺路吗？", lambda t: has(t, "迅达") and has(t, "不能")),
    ("D1", "数据", "2026 年 9 月，迅达快递和顺路快递各有多少笔破损？",
     lambda t: has(t, "迅达", "8", "顺路", "1")),
    ("D2", "数据", "订单 A1679 现在是什么状态？", lambda t: has(t, "待发货")),
    ("D3", "数据", "2026 年 9 月破损最多的是哪家快递，多少笔？", lambda t: has(t, "迅达", "8")),
    ("D4", "数据", "2026 年 9 月一共有多少笔破损？", lambda t: has(t, "9")),
]
# ANCHOR_END: questions

# ---- B 条件的工具：目录、读一节、兜底检索、看结构、只读 SQL -------------------
@tool
def list_docs() -> list[dict]:
    """列出 5 份政策文档和每一节的标题、字数。只返回目录。"""
    docs: dict[str, list] = {}
    for s in store.policy_sections():
        docs.setdefault(s["doc"], []).append({"title": s["title"], "chars": len(s["text"])})
    return [{"doc": k, "sections": v} for k, v in docs.items()]


@tool
def read_section(doc: str, title: str) -> dict:
    """读一节正文；title 用目录里的完整标题，例如“退货和退款 / 退货期限”。"""
    sections = [s for s in store.policy_sections() if s["doc"] == doc]
    match = next((s for s in sections if s["title"] == title), None)
    if match is None:
        raise ToolError(f"{doc} 里没有“{title}”。可选：{'、'.join(s['title'] for s in sections)}")
    return {"doc": doc, "section": match["title"], "text": match["text"], "length": len(match["text"])}


@tool
def search_policies(query: str) -> list[dict]:
    """按两字词片打分的兜底检索，返回最相关的 3 节。"""
    hits = store.search_policies(query)
    if not hits:
        raise ToolError(f"没有找到；可用文档：{', '.join(store.policy_names())}")
    return [{"section": h["title"], "text": h["text"]} for h in hits]


@tool
def list_tables() -> list[dict]:
    """列出数据库里的表和行数。"""
    return [{"table": t, "rows": store.query(f"SELECT COUNT(*) n FROM {t}")[0]["n"]} for t in TABLES]


@tool
def describe_table(name: str) -> dict:
    """看一张表的字段和类型。"""
    if name not in TABLES:
        raise ToolError(f"没有表 {name}；可用：{', '.join(TABLES)}")
    return {"table": name, "columns": [c["name"] for c in store.query(f"PRAGMA table_info({name})")]}


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


B_PROMPT = """你是青石书店的运营助手。政策问题用 list_docs 看目录、read_section 读一节；
数据问题用 list_tables / describe_table 看结构，再写只读 SQL。最多 4 轮，回答不超过 60 字。"""
B_BOX = Toolbox([list_docs, read_section, search_policies, list_tables, describe_table, sql_query])


def ask_a(q: str) -> tuple[str, int]:
    hits = store.search_policies(q)
    ctx = "\n\n".join(f"【{h['title']}】\n{h['text']}" for h in hits) or "（没有检索到条款）"
    prompt = f"只根据下面的资料回答，资料里没有就说“资料里没有”，不要猜。\n问题：{q}\n资料：{ctx}"
    # 和 B 用同样的模型设置：默认思考、不设 max_tokens；两组只差检索方式
    resp = client.chat.completions.create(
        model=MODEL, messages=[{"role": "user", "content": prompt}])
    return resp.choices[0].message.content.strip(), resp.usage.prompt_tokens


def ask_b(q: str) -> tuple[str, int, int, int]:
    messages = [{"role": "system", "content": B_PROMPT}, {"role": "user", "content": q}]
    res = run(messages, B_BOX, model=MODEL, max_turns=5, verbose=False)
    return res.answer or "", res.turns, res.usage.get("prompt_tokens", 0), len(res.steps)


# ANCHOR: main
def main() -> None:
    rows = []
    for name in ("A top-3", "B 按需"):
        for n in range(1, ROUNDS + 1):
            right, turns, tokens, calls, missed = 0, [], [], 0, []
            for qid, kind, q, check in QUESTIONS:
                if name.startswith("A"):
                    answer, used = ask_a(q)
                    t, c = 1, 0
                else:
                    answer, t, used, c = ask_b(q)
                ok = check(answer)
                right += ok
                turns.append(t)
                tokens.append(used)
                calls += c
                if not ok:
                    missed.append(qid)
            rows.append((name, n, right, sum(turns) / len(turns), sum(tokens) // len(tokens),
                         calls, "、".join(missed) or "无"))
            print(f"{name} 第 {n} 轮：答对 {right}/10，平均 {rows[-1][3]:.1f} 轮，"
                  f"平均输入 {rows[-1][4]} token，工具调用 {calls} 次")

    print("\n条件        轮次  答对/10  平均轮数  平均输入 token  工具调用  答错")
    for name, n, right, avg_t, avg_k, calls, missed in rows:
        print(f"{name:11} {n:4}  {right:5}/10  {avg_t:8.1f}  {avg_k:14}  {calls:8}  {missed}")
    print("\nA 只有一次机会；B 按需多轮。看答对、轮数和 token 三列的差别，不要把一次运行当规律。")
# ANCHOR_END: main


if __name__ == "__main__":
    main()
