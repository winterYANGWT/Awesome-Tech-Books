# 14.2 先给目录，再读一节：T22 只要 95 字的“退货期限”。
# mode: run
# expect: 15 ?天
import os

from harness import ToolError, Toolbox, run, tool

from shop import Store

MODEL = os.environ.get("MODEL", "deepseek-flash")
store = Store()

# ANCHOR: tools
@tool
def list_docs() -> list[dict]:
    """列出政策库里的 5 份文档和每一节的标题、字数。只返回目录，不返回正文。"""
    docs: dict[str, list] = {}
    for s in store.policy_sections():
        docs.setdefault(s["doc"], []).append({"title": s["title"], "chars": len(s["text"])})
    return [{"doc": name, "sections": items} for name, items in docs.items()]


@tool
def read_section(doc: str, title: str) -> dict:
    """读一节正文。title 用目录里的完整标题，例如“退货和退款 / 退货期限”。"""
    sections = [s for s in store.policy_sections() if s["doc"] == doc]
    match = next((s for s in sections if s["title"] == title), None)
    if match is None:
        names = "、".join(s["title"] for s in sections) or "（这份文档没有小节）"
        raise ToolError(f"{doc} 里没有“{title}”。可选：{names}")
    return {"doc": doc, "section": match["title"], "text": match["text"], "length": len(match["text"])}


@tool
def search_policies(query: str) -> list[dict]:
    """兜底检索：按两字词片打分返回最相关的 3 节。只在目录里找不到相关小节时用。"""
    hits = store.search_policies(query)
    if not hits:
        raise ToolError(f"没有找到相关条款；可用文档：{', '.join(store.policy_names())}")
    return [{"section": h["title"], "text": h["text"]} for h in hits]
# ANCHOR_END: tools


PROMPT = """你是青石书店的客服。回答政策问题必须先用 list_docs 看目录，再用 read_section 读需要的那一节。
一次只读一节；title 用目录里的完整标题。只有目录里确实没有相关小节时，才用 search_policies 兜底。
回答要指出依据的小节。"""


def main() -> None:
    question = "签收以后多少天内可以退货？"
    messages = [{"role": "system", "content": PROMPT}, {"role": "user", "content": question}]
    res = run(messages, Toolbox([list_docs, read_section, search_policies]), model=MODEL, max_turns=5)
    usage = res.usage
    print(f"\n回答：{res.answer}")
    print(f"轮数 {res.turns}，工具调用 {len(res.steps)} 次，"
          f"输入 {usage.get('prompt_tokens', 0)} token，输出 {usage.get('completion_tokens', 0)} token")
    sections = store.policy_sections()
    whole = sum(len(s["text"]) for s in sections)
    read = next(s for s in sections if s["title"] == "退货和退款 / 退货期限")
    print(f"政策全文 {whole} 字（{len(sections)} 节）；本次只读 1 节 {len(read['text'])} 字")


if __name__ == "__main__":
    main()
