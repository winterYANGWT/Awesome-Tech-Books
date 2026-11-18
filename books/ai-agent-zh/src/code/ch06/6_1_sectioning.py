# mode: run
# output: varies
# expect: 政策
# expect: 汇总
"""T06 的回复草稿，三项检查并行跑，最后按固定顺序汇总。"""
import asyncio
import json
import os
import time

from openai import AsyncOpenAI

from shop import Store
from shop.tools import SUPPORT_PROMPT

client = AsyncOpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")
NO_THINK = {"thinking": {"type": "disabled"}}

DRAFT = (
    "您好，A1659 的《大运河两千年》缺页属于质量问题。"
    "按退货政策“质量问题”一节，质量问题退全款（书款加运费），签收后 60 天内可以申请；"
    "您这张订单在 11 月 11 日签收，还在期限内。"
    "订单实付 82 元，我们会为您安排全额退款，1–3 个工作日原路退回。"
    "请保留外包装，不需要寄回。给您添麻烦了。"
)

store = Store()
POLICY = "\n\n".join(f"## {x['title']}\n{x['text']}"
                    for x in store.search_policies("质量问题 退全款 签收后 60 天"))
PRIVACY = store.policy("privacy")

CHECKS = [
    {"name": "政策", "rule": "只核对回复是否符合下面的政策条款；evidence 必须抄条款里完整的一句（至少 12 个字）。",
     "context": POLICY},
    {"name": "语气", "rule": "只检查是否简洁、礼貌、给了下一步；不要判断政策对错。",
     "context": SUPPORT_PROMPT},
    {"name": "隐私", "rule": "只检查是否泄露其他顾客的信息或索要了不该问的证件信息。",
     "context": PRIVACY},
]

SYSTEM = """你是客服回复的单项检查器，只按给定的检查项判断，不替其他检查项下结论。
输出 JSON：{"pass": true 或 false, "issues": ["问题"], "evidence": "草稿原句或政策条款标题"}。
没有问题时 issues 是空数组；evidence 必须能在草稿或给定条款里找到。"""


def load_json(text: str) -> dict:
    text = text.strip()
    if text.startswith("```"):
        text = text.split("\n", 1)[1].rsplit("```", 1)[0]
    data = json.loads(text[text.find("{"):text.rfind("}") + 1])
    return {"pass": data.get("pass") is True, "issues": list(data.get("issues") or []),
            "evidence": str(data.get("evidence") or "")}


async def check(spec: dict) -> dict:
    r = await client.chat.completions.create(
        model=MODEL,
        messages=[{"role": "system", "content": SYSTEM},
                  {"role": "user", "content": f"检查项：{spec['name']}\n要求：{spec['rule']}\n\n"
                                              f"参考材料：\n{spec['context']}\n\n待检查的草稿：\n{DRAFT}"}],
        response_format={"type": "json_object"}, max_tokens=400, extra_body=NO_THINK)
    return {"result": load_json(r.choices[0].message.content), "usage": r.usage}


async def main() -> None:
    started = time.monotonic()
    results = await asyncio.gather(*(check(spec) for spec in CHECKS))
    elapsed = time.monotonic() - started

    print("三项检查（并行发出，按输入顺序打印）")
    ok = 0
    for spec, item in zip(CHECKS, results):
        res = item["result"]
        ok += res["pass"]
        issues = "；".join(res["issues"]) if res["issues"] else "无"
        print(f"{spec['name']}  通过={res['pass']}  issues={issues}  evidence={res['evidence'][:48]}")

    # 汇总规则是代码：全过才通过；否则按 政策 > 隐私 > 语气 排序，交回生成步。
    print(f"汇总：{'通过' if ok == len(CHECKS) else '不通过'}（{ok}/{len(CHECKS)} 项通过）")
    if ok != len(CHECKS):
        for spec, item in sorted(zip(CHECKS, results), key=lambda x: {"政策": 0, "隐私": 1, "语气": 2}[x[0]["name"]]):
            for issue in item["result"]["issues"]:
                print(f"  回给生成步：{spec['name']} / {issue}")

    u = [item["usage"] for item in results]
    prompt = sum(x.prompt_tokens for x in u)
    completion = sum(x.completion_tokens for x in u)
    print(f"token：prompt {prompt} + completion {completion} = {prompt + completion}；并行用时 {elapsed:.1f} 秒")


asyncio.run(main())
