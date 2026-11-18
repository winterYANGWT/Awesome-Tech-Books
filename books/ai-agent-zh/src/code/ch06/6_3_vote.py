# mode: run
# output: varies
# expect: 票型
# expect: 多数票
"""T14 教辅拆封：5 个不同角度的评审各投一票，代码计票。"""
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

RETURNS = Store.policy("returns")
CASE = f"""今天是 2026-11-17。
顾客原话：《小学语文阅读训练》拆开做了两页发现不是孩子要的，订单 A1649，退一下。
订单事实：A1649，2026-11-05 签收；2 本《小学语文阅读训练》，分类是教辅，实付 64 元（含运费 6 元）。
政策原文：
{RETURNS}"""

REVIEWERS = [
    ("严格按政策", "逐条对照政策原文，只按条款判断，不替顾客争取。"),
    ("顾客视角", "从顾客处境复述一遍：他买到的到底是不是他要的；判断仍以条款为准。"),
    ("只看金额", "你只算钱：退的话退多少、运费谁承担。金额算得通就投可退；商品类型不是你的检查项。"),
    ("只看时限", "你只看日期：签收几天、在不在 15 天或 60 天里。在时限内就投可退；商品类型不是你的检查项。"),
    ("找反例", "先假设“可以退”，然后到政策里找一条能推翻它的条款；找不到才投可退。"),
]

SYSTEM = """你是退货判断的评审。只输出 JSON：
{"verdict": "可退" 或 "不可退" 或 "需人工", "reason": "不超过 50 字，引用政策或顾客原话"}。"""


def load_json(text: str) -> dict:
    text = text.strip()
    if text.startswith("```"):
        text = text.split("\n", 1)[1].rsplit("```", 1)[0]
    data = json.loads(text[text.find("{"):text.rfind("}") + 1])
    verdict = data.get("verdict")
    if verdict not in ("可退", "不可退", "需人工"):
        verdict = "需人工"
    return {"verdict": verdict, "reason": " ".join(str(data.get("reason") or "").split())}


async def vote(index: int, name: str, angle: str) -> dict:
    r = await client.chat.completions.create(
        model=MODEL,
        messages=[{"role": "system", "content": SYSTEM},
                  {"role": "user", "content": f"你是第 {index} 位评审，角度是“{name}”：{angle}\n\n{CASE}"}],
        response_format={"type": "json_object"}, max_tokens=300, extra_body=NO_THINK)
    return {"reviewer": index, "name": name, **load_json(r.choices[0].message.content),
            "usage": r.usage}


def majority(votes: list[str]) -> tuple[str, Counter]:
    c = Counter(votes)
    top, n = c.most_common(1)[0]
    if n >= 3 and sum(1 for v in c.values() if v == n) == 1:
        return top, c
    return "需人工", c


async def main() -> None:
    started = time.monotonic()
    votes = await asyncio.gather(*(vote(i, name, angle)
                                   for i, (name, angle) in enumerate(REVIEWERS, 1)))
    elapsed = time.monotonic() - started

    print("T14 投票：5 位评审，各自看同一个案子")
    for v in votes:
        print(f"评审 {v['reviewer']}（{v['name']}）  {v['verdict']}  {v['reason']}")

    result, counts = majority([v["verdict"] for v in votes])
    print(f"票型：{' / '.join(f'{k} {n}' for k, n in counts.most_common())}")
    print(f"多数票：{result}")
    prompt = sum(v["usage"].prompt_tokens for v in votes)
    completion = sum(v["usage"].completion_tokens for v in votes)
    print(f"token：{prompt + completion}；用时 {elapsed:.1f} 秒")


asyncio.run(main())
