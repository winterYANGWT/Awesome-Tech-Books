# 7.2 同一个评论子任务，模糊委派和具体委派各跑 2 次。
# mode: run
# expect: 引用
import json
import os
import re

from openai import OpenAI

from shop import Store

client = OpenAI()                                  # 读 OPENAI_BASE_URL、OPENAI_API_KEY
MODEL = os.environ.get("MODEL", "deepseek-flash")
NO_THINK = {"thinking": {"type": "disabled"}}
reviews = Store().reviews_of("B12", 5)
REVIEW_TEXT = "\n".join(f"{r['id']} {r['stars']} 星：{r['text']}（{r['created']}）" for r in reviews)

# ANCHOR: spec
VAGUE = """看看下面这些 B12 的评论，告诉我顾客觉得怎么样。这是第 {n}/2 次。
{data}"""

SPECIFIC = """目标：总结 B12《大运河两千年》的这些评论，回答“有没有人提到缺页或装订问题”。
输入：book_id=B12。工具结果（只读，不要改）：
{data}
输出：只输出 JSON，字段为
{{"book_id": "B12", "review_ids": ["V0000"], "summary": "不超过 120 字的摘要"}}
要求：review_ids 列出你实际用到的评论 id；summary 里引用原句时用引号；不要猜没有出现的评论；
不要联系顾客；查完上面这些评论（最多 5 条）就停。这是第 {n}/2 次。"""


# ANCHOR_END: spec
def ask(prompt: str) -> str:
    resp = client.chat.completions.create(
        model=MODEL, messages=[{"role": "user", "content": prompt}], max_tokens=400,
        extra_body=NO_THINK)
    return resp.choices[0].message.content.strip()


# ANCHOR: metrics
def metrics(text: str) -> tuple[int, int, bool]:
    ids = set(re.findall(r"V\d{4}", text))
    quoted = any(r["text"][:6] in text for r in reviews if len(r["text"]) >= 6)
    return len(text), len(ids), quoted


# ANCHOR_END: metrics
def main() -> None:
    rows = {}
    for name, template in (("模糊版", VAGUE), ("具体版", SPECIFIC)):
        print(name)
        rows[name] = []
        for n in (1, 2):
            text = ask(template.format(n=n, data=REVIEW_TEXT))
            length, ids, quoted = metrics(text)
            rows[name].append((ids, quoted))
            print(f"  第 {n} 次：长度 {length} 字，review_id {ids} 个，引用原文 {'是' if quoted else '否'}")
            print(f"    输出：{' '.join(text.split())[:100]}")
    for name, items in rows.items():
        ids = sum(1 for i, _ in items if i)
        quoted = sum(1 for _, q in items if q)
        print(f"  {name}：{ids}/2 带 review_id，{quoted}/2 引用原文")


if __name__ == "__main__":
    main()
