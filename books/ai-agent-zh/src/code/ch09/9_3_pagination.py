# 9.3 历史类里 60 元以下的书：只给前 5 条，和给 next_cursor 让模型自己翻页。
# mode: run
# output: varies
# expect: next_cursor
# expect: 完整
# timeout: 600
"""1 个问题 × 2 种返回 × 6 次：A 组没有 cursor，B 组每页 3 条、可翻页。"""
import base64
import json

from harness import Toolbox, run, tool

from shop import Store

NO_THINK = {"thinking": {"type": "disabled"}}
QUESTION = "历史类里 60 元以下的书有哪些？"
TITLES = ["宋朝的日常", "唐诗里的长安", "中国茶简史"]      # 全部 60 元以下的历史书
SYSTEM = """你是青石书店的店员。用 list_books 按分类查书。
如果结果里有 next_cursor，就用它继续翻页，直到 has_more 为 false。
回答顾客的问题，列出书名和价格，不要列不符合条件的书。"""


# ANCHOR: page
def encode(start: int) -> str:
    return base64.urlsafe_b64encode(str(start).encode()).decode().rstrip("=")


def decode(cursor: str) -> int:
    try:
        return int(base64.urlsafe_b64decode(cursor + "=" * (-len(cursor) % 4)))
    except (ValueError, TypeError):
        return 0


def make_box(store: Store, with_cursor: bool) -> Toolbox:
    @tool
    def list_books(category: str, limit: int = 5, cursor: str = "") -> dict:
        """按分类列书，价格从高到低；limit 是每页条数。"""
        name = category.removesuffix("类")
        rows = store.query("SELECT id, title, price FROM books WHERE category = ? "
                           "ORDER BY price DESC, id", name)
        size = max(1, min(limit, 3 if with_cursor else 5))   # B 组每页封顶 3 条
        start = decode(cursor) if with_cursor else 0
        page = rows[start:start + size]
        if not with_cursor:
            return {"books": page}                 # 坏设计：不告诉模型还有没有
        out = {"books": page, "total": len(rows)}
        end = start + len(page)
        out["has_more"] = end < len(rows)
        if out["has_more"]:
            out["next_cursor"] = encode(end)
        return out

    box = Toolbox([list_books])
    if with_cursor:
        box.tools["list_books"].description += " 服务端每页最多 3 条。"
    return box
# ANCHOR_END: page


def once(with_cursor: bool) -> dict:
    res = run([{"role": "system", "content": SYSTEM}, {"role": "user", "content": QUESTION}],
              make_box(Store(), with_cursor), max_turns=8, verbose=False,
              extra_body=NO_THINK, max_tokens=400)
    seen = set()
    for step in res.steps:
        if step.name == "list_books":
            seen |= {b["id"] for b in json.loads(step.result)["books"]}
    answer = res.answer or ""
    return {"seen": len(seen), "calls": sum(s.name == "list_books" for s in res.steps),
            "done": all(t in answer for t in TITLES),
            "prompt": res.usage.get("prompt_tokens", 0), "turns": res.turns}


# ANCHOR: main
def main() -> None:
    print(f"问题：{QUESTION}（符合条件的 {len(TITLES)} 本：{'、'.join(TITLES)}）")
    print("组             完整   平均看到  平均查询  平均轮数  平均输入 token")
    for label, flag in (("A 无 cursor", False), ("B 有 next_cursor", True)):
        rows = [once(flag) for _ in range(6)]
        n = len(rows)
        avg = lambda k: sum(r[k] for r in rows) / n
        print(f"{label:<14} {sum(r['done'] for r in rows)}/{n}    "
              f"{avg('seen'):.1f}/7    {avg('calls'):.1f}       {avg('turns'):.1f}       "
              f"{avg('prompt'):.0f}")
    print("说明：A 组不知道还有第 6、7 条；B 组每页 3 条，翻到第 3 页才看全。")


if __name__ == "__main__":
    main()
# ANCHOR_END: main
