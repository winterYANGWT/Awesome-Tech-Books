# mode: run
# output: varies
# expect: 单票
# expect: 多数票
# expect: 五票全错
"""8 个政策问题 × 5 位评审：同一批票，分别按“单票”和“多数票”统计。"""
import asyncio
import json
import os
import re
import time
from collections import Counter

from openai import AsyncOpenAI

from shop import Store

client = AsyncOpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")
NO_THINK = {"thinking": {"type": "disabled"}}
store = Store()

QUESTIONS = [
    {"id": "Q1", "label": "退货期限", "q": "顾客 2026-06-20 签收一本普通书，2026-11-17 想按“不想要了”退货，能退吗？",
     "options": ["能", "不能", "需主管批准", "看情况"], "answer": "B", "policy": "returns"},
    {"id": "Q2", "label": "质量问题期限", "q": "顾客 2026-09-30 签收，2026-11-17 发现缺页，能按质量问题退全款吗？",
     "options": ["能", "不能", "只能退书款", "需主管批准"], "answer": "A", "policy": "returns"},
    {"id": "Q3", "label": "包邮门槛", "q": "订单实付 98 元，普通会员，需要付运费吗？",
     "options": ["要，6 元", "不要", "要，10 元", "看会员等级"], "answer": "A", "policy": "shipping"},
    {"id": "Q4", "label": "不满包邮运费", "q": "订单实付 98 元，普通会员，运费是多少？",
     "options": ["0 元", "6 元", "10 元", "12 元"], "answer": "B", "policy": "shipping"},
    {"id": "Q5", "label": "100 积分", "q": "100 积分能抵多少元？",
     "options": ["0.5 元", "2 元", "1 元", "5 元"], "answer": "C", "policy": "membership"},
    {"id": "Q6", "label": "银卡生日券", "q": "银卡会员生日当月能领多少元的券？",
     "options": ["10 元", "20 元", "30 元", "50 元"], "answer": "B", "policy": "membership"},
    {"id": "Q7", "label": "主管门槛", "q": "A1680 实付 680 元，装订脱落，客服能直接退全款吗？",
     "options": ["能，质量问题", "不能，超过 500 元要主管", "能，但只能退书款", "不能，超过 60 天"],
     "answer": "B", "policy": "returns"},
    {"id": "Q8", "label": "广东快递", "q": "广东的订单备注“请用顺路快递”，客服能答应吗？",
     "options": ["能", "不能，默认迅达快递", "能，加 6 元", "只能到付"], "answer": "B",
     "policy": "shipping"},
]

REVIEWERS = [
    ("快速作答", "不要逐条核对，按你记得的常见说法快速选。"),
    ("逐条核对", "逐条对照政策原文，特别注意日期和修订记录。"),
    ("顾客视角", "按对顾客最有利的理解选，但仍要能在原文里找到依据。"),
    ("只看数字", "把选项里的数字、单位与原文逐项对照，忽略无关段落。"),
    ("找反例", "先假设最顺眼的选项是错的，找出原文里能推翻它的那句。"),
]

SYSTEM = """你是政策问答的评审。只输出 JSON：{"choice": "A" 或 "B" 或 "C" 或 "D", "reason": "不超过 30 字"}。"""


def context_for(doc: str) -> str:
    return Store.policy(doc)


def load_json(text: str) -> dict:
    text = text.strip()
    if text.startswith("```"):
        text = text.split("\n", 1)[1].rsplit("```", 1)[0]
    data = json.loads(text[text.find("{"):text.rfind("}") + 1])
    m = re.search(r"[ABCD]", str(data.get("choice", "")).upper())
    return {"choice": m.group(0) if m else "?", "reason": " ".join(str(data.get("reason") or "").split())}


async def vote(q: dict, index: int, name: str, angle: str, sem: asyncio.Semaphore) -> dict:
    options = "\n".join(f"{chr(65 + i)}. {x}" for i, x in enumerate(q["options"]))
    async with sem:
        r = await client.chat.completions.create(
            model=MODEL,
            messages=[{"role": "system", "content": SYSTEM},
                      {"role": "user", "content": f"你是第 {index} 位评审，角度是“{name}”：{angle}\n\n"
                                                  f"政策材料：\n{context_for(q['policy'])}\n\n"
                                                  f"问题（{q['id']}）：{q['q']}\n{options}"}],
            response_format={"type": "json_object"}, max_tokens=200, extra_body=NO_THINK)
    return {**load_json(r.choices[0].message.content), "q": q["id"], "reviewer": index,
            "usage": r.usage}


def majority(votes: list[str]) -> tuple[str | None, Counter]:
    c = Counter(votes)
    top, n = c.most_common(1)[0]
    if n >= 3 and sum(1 for v in c.values() if v == n) == 1:
        return top, c
    return None, c


async def main() -> None:
    started = time.monotonic()
    sem = asyncio.Semaphore(8)
    jobs = [vote(q, i, name, angle, sem)
            for q in QUESTIONS for i, (name, angle) in enumerate(REVIEWERS, 1)]
    results = await asyncio.gather(*jobs)
    elapsed = time.monotonic() - started

    by_q = {q["id"]: [r for r in results if r["q"] == q["id"]] for q in QUESTIONS}
    single_ok = majority_ok = ties = all_wrong = 0
    print("8 个政策问题，每题 5 票")
    for q in QUESTIONS:
        votes = [r["choice"] for r in by_q[q["id"]]]
        maj, _ = majority(votes)
        single_ok += votes[0] == q["answer"]
        majority_ok += maj == q["answer"]
        ties += maj is None
        all_wrong += all(v != q["answer"] for v in votes)
        single_mark = "对" if votes[0] == q["answer"] else "错"
        maj_mark = "对" if maj == q["answer"] else ("平" if maj is None else "错")
        print(f"{q['id']} {q['label']}  票 {' '.join(votes)}  单票 {votes[0]}({single_mark})  "
              f"多数 {maj or '平票'}({maj_mark})  正确 {q['answer']}")

    prompt = sum(r["usage"].prompt_tokens for r in results)
    completion = sum(r["usage"].completion_tokens for r in results)
    print(f"单票答对 {single_ok}/8；多数票答对 {majority_ok}/8；平票/无多数 {ties}；五票全错 {all_wrong}")
    print(f"token：prompt {prompt} + completion {completion} = {prompt + completion}；"
          f"平均每次 completion {completion / len(results):.0f}；40 次调用，用时 {elapsed:.1f} 秒")


asyncio.run(main())
