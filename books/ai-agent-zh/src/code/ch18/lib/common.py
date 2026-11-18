"""第 18 章共用：调研线的工具、子 agent 的 JSON 合同、引用检查。

共享代码放在 `ch18/lib/` 子目录，避免被 checker 当成一个小节示例。
"""
from __future__ import annotations

import json
import re
import threading
from typing import Annotated, Literal

from pydantic import BaseModel, ValidationError

from harness import ToolError, Toolbox, tool
from shop import ShopError, Store

store = Store()
DB_LOCK = threading.Lock()   # 一个 SQLite 连接不能同时被三个线程用

# ANCHOR: sql_tool
@tool
def readonly_sql(sql: Annotated[str, "一条 SQLite SELECT 语句；先聚合再下钻，不要 SELECT *"],
                 max_rows: int = 30) -> dict:
    """执行一条只读 SQL。只允许 SELECT，最多返回 max_rows 行；被截断时给出提示。"""
    try:
        with DB_LOCK:
            rows = store.readonly_sql(sql, max_rows + 1)
    except ShopError as e:
        raise ToolError(f"{e}。只允许 SELECT；写操作会被数据库拒绝") from None
    truncated = len(rows) > max_rows
    rows = rows[:max_rows]
    out = {"columns": list(rows[0]) if rows else [], "rows": rows,
           "returned": len(rows), "truncated": truncated}
    if truncated:
        out["hint"] = "还有更多行：加 WHERE、GROUP BY 或 LIMIT，不要 SELECT *"
    return out
# ANCHOR_END: sql_tool


@tool
def list_reviews(book_id: Annotated[str, "书号，例如 B12"]) -> dict:
    """读一本书的全部评论：总条数、星级、正文和日期。条数少时要在结论里写样本缺口。"""
    with DB_LOCK:
        if store.book(book_id) is None:
            raise ToolError(f"书 {book_id} 不存在")
        rows = store.query("SELECT stars, text, created FROM reviews WHERE book_id = ? "
                           "ORDER BY created, id", book_id)
    return {"book_id": book_id, "total": len(rows), "reviews": rows[:12]}


@tool
def search_policies(query: Annotated[str, "要查的问题，例如“退货期限什么时候改的”"]) -> list[dict]:
    """在政策文档里查相关条款，返回最相关的 3 节原文。"""
    hits = store.search_policies(query)
    if not hits:
        raise ToolError("没有找到相关条款；换短一点的关键词再查")
    return [{"section": h["title"], "text": h["text"]} for h in hits]


@tool
def read_section(doc: Annotated[str, "文档名，不含 .md，例如 returns"],
                 title: Annotated[str, "完整小节标题，例如“退货和退款 / 退货期限”"]) -> dict:
    """读一节政策正文；标题写错时列出这份文档可选的小节。"""
    sections = [s for s in store.policy_sections() if s["doc"] == doc]
    if not sections:
        raise ToolError(f"没有文档 {doc}；可选：{', '.join(store.policy_names())}")
    match = next((s for s in sections if s["title"] == title), None)
    if match is None:
        raise ToolError(f"{doc} 里没有“{title}”；可选：{'；'.join(s['title'] for s in sections)}")
    return {"doc": doc, "section": match["title"], "text": match["text"], "length": len(match["text"])}


SQL_BOX = Toolbox([readonly_sql])
REVIEW_BOX = Toolbox([list_reviews])
POLICY_BOX = Toolbox([search_policies, read_section])


class Finding(BaseModel):
    claim: str
    evidence: str
    source: str


class ResearchResult(BaseModel):
    findings: list[Finding]
    gaps: list[str] = []
    status: Literal["ok", "failed"] = "ok"


RESEARCH_PROMPT = """你是调研子 agent，只负责一条调查线。用给定的工具查，不要猜，不要查别的线。
最多调用 4 次工具，然后立即停止调用工具，只输出一个 JSON 对象，不要代码块、不要解释：
{"findings": [{"claim": "一句结论", "evidence": "数字或原文", "source": "表名+字段，或文档名+小节"}],
 "gaps": ["找不到或样本不足的地方"], "status": "ok"}
claim 里必须有数字或明确的事实；source 写清楚是哪张表、哪个字段、哪份文档。"""


def parse_json(text: str) -> dict:
    """从回答里取出 JSON；模型偶尔会包一层 ```json。"""
    text = (text or "").strip()
    text = re.sub(r"^```(?:json)?\s*|\s*```$", "", text)
    start, end = text.find("{"), text.rfind("}")
    if start < 0 or end < start:
        raise ValueError("回答里没有 JSON 对象")
    return json.loads(text[start:end + 1])


def parse_result(text: str) -> ResearchResult:
    try:
        obj = parse_json(text)
        if obj.get("status") not in ("ok", "failed"):
            obj["status"] = "ok" if obj.get("findings") else "failed"
        return ResearchResult.model_validate(obj)
    except (ValueError, ValidationError) as e:
        raise ValueError(f"子 agent 的 JSON 不合格：{e}") from None


class UncitedClaim(Exception):
    """报告里有句子没有数字或没有来源。"""


NUM = re.compile(r"\d+(?:\.\d+)?")
MARK = re.compile(r"\[(数据|政策|评论)")


def _numbers(text: str) -> list[str]:
    return [n.rstrip(".") for n in NUM.findall(text.replace(",", ""))]


def check_citations(report: str, findings: list[dict]) -> int:
    """逐段检查：每段至少一个数字、一个来源标记，数字必须出现在 findings 里。

    findings 是 18.3 那种 [{"claim","evidence","source"}, …] 的列表。
    返回检查过的段落数；不合格抛 UncitedClaim。
    """
    allowed = set()
    for f in findings:
        allowed.update(_numbers(json.dumps(f, ensure_ascii=False)))
    paras = [p.strip() for p in re.split(r"\n\s*\n", report.strip()) if p.strip()]
    if not paras:
        raise UncitedClaim("报告是空的")
    for para in paras:
        sentence = re.split(r"(?<=[。！？])", para.strip())[0].strip()
        nums = _numbers(para)
        if not nums:
            raise UncitedClaim(f"“{sentence.rstrip('。！？')}”没有数据来源")
        if not MARK.search(para):
            raise UncitedClaim(f"“{sentence.rstrip('。！？')}”没有标注来源")
        for n in nums:
            if n not in allowed:
                raise UncitedClaim(f"“{sentence.rstrip('。！？')}”里的 {n} 不在子 agent 的 findings 里")
    return len(paras)
