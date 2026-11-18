# mode: run
# output: varies
# expect: 失败
# expect: 保留
"""三个检查并行，其中一个在协程里抛异常；return_exceptions=True 保住另外两个。"""
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

SYSTEM = """你是客服回复的单项检查器，只按给定的检查项判断。
输出 JSON：{"pass": true 或 false, "issues": ["问题"], "evidence": "草稿原句或政策条款标题"}。"""


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
    if spec["name"] == "语气":
        # 故意让这一项在拿到回复之后崩掉：模拟检查器加载评分表失败。
        raise ValueError("语气检查器崩溃：评分表 vocab.json 读不出来")
    return {"result": load_json(r.choices[0].message.content), "usage": r.usage}


async def main() -> None:
    started = time.monotonic()
    # 关键：return_exceptions=True。默认 False 时，第一个异常立刻抛给 await，其余结果拿不到。
    results = await asyncio.gather(*(check(spec) for spec in CHECKS), return_exceptions=True)
    elapsed = time.monotonic() - started

    print("三个检查（并行发出，按输入顺序打印）")
    kept, missing = [], []
    for spec, res in zip(CHECKS, results):
        if isinstance(res, BaseException):
            missing.append(spec["name"])
            print(f"{spec['name']}  error  失败：{res}")
        else:
            kept.append(spec["name"])
            print(f"{spec['name']}  ok  通过={res['result']['pass']}  "
                  f"issues={'；'.join(res['result']['issues']) or '无'}")
    print(f"汇总：失败（{len(missing)} 项未完成：{'、'.join(missing)}）→ 需人工/重试一次")
    print(f"已保留：{'、'.join(kept)} 的结论；没有丢掉另外两项")

    usage = [r["usage"] for r in results if not isinstance(r, BaseException)]
    prompt = sum(x.prompt_tokens for x in usage)
    completion = sum(x.completion_tokens for x in usage)
    print(f"token：prompt {prompt} + completion {completion} = {prompt + completion}；用时 {elapsed:.1f} 秒")


asyncio.run(main())
