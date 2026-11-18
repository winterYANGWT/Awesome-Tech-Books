# 9.3 反例：只返回前 5 条、不说还有多少，模型以为已经看全了。
# mode: fail
# llm: none
# expect: 静默截断
# expect: next_cursor
"""检查 list_books 的返回结构：有剩余条目却没有 next_cursor / has_more，就失败。"""
import sys

from shop import Store

store = Store()


# ANCHOR: tool
def list_books(category: str, limit: int = 5) -> dict:
    """按分类列书，价格从高到低，最多 5 条。"""
    rows = store.query("SELECT id, title, price FROM books WHERE category = ? "
                       "ORDER BY price DESC, id LIMIT ?", category, min(limit, 5))
    return {"books": rows}
# ANCHOR_END: tool


def main() -> None:
    result = list_books("历史")
    print(f"list_books('历史') 返回 {len(result['books'])} 条："
          + "、".join(f"{b['id']} {b['title']} {b['price']:.0f} 元" for b in result["books"]))
    total = store.query("SELECT COUNT(*) AS n FROM books WHERE category = '历史'")[0]["n"]
    print(f"库里实际有 {total} 条")
    print("返回里的键：" + ", ".join(sorted(result)))
    left = total - len(result["books"])
    if left and not (result.get("next_cursor") or result.get("has_more")):
        # ANCHOR: check
        print(f"结果被静默截断：还有 {left} 条，没有 next_cursor")
        sys.exit(1)
        # ANCHOR_END: check


if __name__ == "__main__":
    main()
