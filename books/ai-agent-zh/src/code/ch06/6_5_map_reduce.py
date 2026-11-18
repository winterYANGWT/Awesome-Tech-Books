# mode: run
# output: varies
# expect: 汇总
"""B07 的 10 条评论：每条一次 map，按 review_id 排序后 reduce。"""
import asyncio
import json
import os
import time
from collections import Counter

from openai import AsyncOpenAI

from shop import Store

client = AsyncOpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")
NO_THINK = {"thinking": {"type": "disabled"}}
ISSUES = ("缺页/装订", "运输破损", "内容/版本", "无问题", "其他")


def load_json(text: str) -> dict:
    text = text.strip()
    if text.startswith("```"):
        text = text.split("\n", 1)[1].rsplit("```", 1)[0]
    return json.loads(text[text.find("{"):text.rfind("}") + 1])


async def map_one(review: dict, rid: str) -> dict:
    r = await client.chat.completions.create(
        model=MODEL,
        messages=[
            {"role": "system", "content": "你是评论标注器，只读给出的这一条评论，不参考别的评论。"},
            {"role": "user", "content": f"""输出 JSON：{{"review_id": "{rid}", "issue": "缺页/装订" 或 "运输破损" 或 "内容/版本" 或 "无问题" 或 "其他", "stars": 星级整数, "quote": "评论里的一句原话"}}
只标这条评论本身说的内容，不要推断其他评论。

review_id：{rid}
星级：{review['stars']}
评论：{review['text']}"""},
        ],
        response_format={"type": "json_object"}, max_tokens=250, extra_body=NO_THINK)
    data = load_json(r.choices[0].message.content)
    issue = data.get("issue") if data.get("issue") in ISSUES else "其他"
    quote = " ".join(str(data.get("quote") or "").split())
    if not quote or quote not in review["text"]:
        quote = review["text"]  # 模型改写了原句时，退回原文
    return {"review_id": rid, "issue": issue, "stars": review["stars"],
            "quote": quote, "usage": r.usage}


async def main() -> None:
    started = time.monotonic()
    store = Store()
    reviews = store.reviews_of("B07", limit=10)
    parts = await asyncio.gather(*(map_one(r, f"B07-R{i:02d}")
                                   for i, r in enumerate(reviews, 1)))
    parts = sorted(parts, key=lambda p: p["review_id"])

    print(f"map：{len(parts)} 片全部返回，按 review_id 排序（完成顺序不保证）")
    for p in parts:
        print(f"{p['review_id']}  {p['stars']} 星  {p['issue']}  “{p['quote'][:30]}”")

    # reduce 只读 map 的结构化结果，不读评论原文。
    compact = [{k: p[k] for k in ("review_id", "issue", "stars", "quote")} for p in parts]
    counts = Counter(p["issue"] for p in compact)
    ordered = sorted(compact, key=lambda p: (p["stars"], p["review_id"]))
    quotes = [p for p in ordered if p["issue"] != "无问题"][:2]
    quotes += [p for p in ordered if p not in quotes][:2 - len(quotes)]
    print("reduce 汇总（代码计数）：" + "；".join(f"{k} {n}" for k, n in
                                          sorted(counts.items(), key=lambda x: (-x[1], x[0]))))
    for i, p in enumerate(quotes, 1):
        print(f"引用 {i}（{p['review_id']}，{p['stars']} 星）：{p['quote']}")

    r = await client.chat.completions.create(
        model=MODEL,
        messages=[{"role": "system", "content": "你只根据给定的结构化计数和引用写两句话，不要补充原文里没有的内容。"},
                  {"role": "user", "content": "B07 的评论汇总（map 结果）：\n"
                                              + json.dumps({"counts": counts, "quotes": quotes},
                                                           ensure_ascii=False, indent=1)}],
        max_tokens=200, extra_body=NO_THINK)
    print("reduce 摘要（模型）：" + " ".join(r.choices[0].message.content.split()))

    usage = [p["usage"] for p in parts] + [r.usage]
    prompt = sum(u.prompt_tokens for u in usage)
    completion = sum(u.completion_tokens for u in usage)
    print(f"token：{prompt + completion}（{len(usage)} 次调用）；map+reduce 用时 {time.monotonic() - started:.1f} 秒")


asyncio.run(main())
