# 7.1 编排者先提交计划，三个工作者并行，最后汇总并带引用。
# mode: run
# expect: 订单.*政策
import asyncio
import json
import os
import re

from openai import OpenAI

from shop import Store

client = OpenAI()                                  # 读 OPENAI_BASE_URL、OPENAI_API_KEY
MODEL = os.environ.get("MODEL", "deepseek-flash")
NO_THINK = {"thinking": {"type": "disabled"}}
store = Store()

ROLES = ["订单查询", "政策查询", "评论查询", "回复草稿"]
# ANCHOR: plan
PLAN_TOOL = {
    "type": "function",
    "function": {
        "name": "submit_plan",
        "description": "提交工单处理计划。只提交任务，不执行任何操作。",
        "parameters": {
            "type": "object",
            "properties": {
                "tasks": {
                    "type": "array", "minItems": 1, "maxItems": 4,
                    "items": {
                        "type": "object",
                        "properties": {
                            "id": {"type": "string", "enum": ["T1", "T2", "T3", "T4"]},
                            "role": {"type": "string", "enum": ROLES},
                            "goal": {"type": "string", "minLength": 4},
                            "inputs": {
                                "type": "object",
                                "properties": {
                                    "order_id": {"type": "string"},
                                    "book_id": {"type": "string"},
                                    "customer_id": {"type": "string"},
                                    "depends_on": {"type": "array", "items": {"type": "string"}},
                                },
                                "additionalProperties": False,
                            },
                            "output_schema": {"type": "string",
                                              "enum": ["order_facts", "policy_clauses",
                                                       "review_digest", "reply_draft"]},
                        },
                        "required": ["id", "role", "goal", "inputs", "output_schema"],
                    },
                }
            },
            "required": ["tasks"],
        },
    },
}

ORCHESTRATOR = """你是青石书店的工单编排者。你只提交计划，不退款、不改地址、不联系顾客。
可用工作者与能力：
- 订单查询：订单状态、实付、已退、商品、物流
- 政策查询：退货、配送、会员等政策条款
- 评论查询：一本书的顾客评论
- 回复草稿：根据其他任务的结果写回复
为工单拆 2–4 个任务，必须覆盖订单事实、退货政策、书的评论。不要派回复草稿，汇总者会写。"""


def plan_for(ticket: str) -> dict:
    resp = client.chat.completions.create(
        model=MODEL,
        messages=[{"role": "system", "content": ORCHESTRATOR}, {"role": "user", "content": ticket}],
        tools=[PLAN_TOOL],
        tool_choice={"type": "function", "function": {"name": "submit_plan"}},
        extra_body={"thinking": {"type": "disabled"}},
    )
    call = resp.choices[0].message.tool_calls[0]
    print("submit_plan 已提交（编排者只有这个工具，没有退款/改地址工具）")
    return json.loads(call.function.arguments)


# ANCHOR_END: plan
# ANCHOR: worker
def fetch(task: dict):
    """示例里由代码先执行只读查询，把结果和 inputs 一起拼进 worker 的提示；结果不写回 inputs。"""
    role, inp = task["role"], task["inputs"]
    if role == "订单查询":
        o = store.order(inp["order_id"])
        return {"order_id": o["id"], "status": o["status"], "total_paid": o["total"],
                "refunded": o["refunded"], "delivered": o["shipment"]["delivered"],
                "items": [{"title": i["title"], "qty": i["qty"], "paid": round(i["price"] * o["discount"], 2)}
                          for i in o["items"]]}
    if role == "政策查询":
        return store.search_policies(task["goal"])
    if role == "评论查询":
        bid = inp.get("book_id", "B12")
        if not re.fullmatch(r"B\d{2}", bid):
            bid = store.search_books(bid)[0]["id"]
        return store.reviews_of(bid)
    raise ValueError(f"未知角色 {role}")


SCHEMA = {
    "订单查询": '{"order_id": str, "status": str, "total_paid": number, "refunded": number, '
                '"delivered": str, "items": [{"title": str, "qty": int, "paid": number}]}',
    "政策查询": '{"clauses": [{"policy_section": str, "text": str}]}',
    "评论查询": '{"book_id": str, "review_ids": [str], "summary": str}',
}


def call_worker(task: dict) -> dict:
    data = fetch(task)
    prompt = f"""你是{task['role']}工作者。你只看到下面这一条委派，看不到主对话。
目标：{task['goal']}
输入：{json.dumps(task['inputs'], ensure_ascii=False)}
工具结果：{json.dumps(data, ensure_ascii=False)}
要求：只根据工具结果回答，不猜；不退款、不联系顾客；输出一个 JSON 对象，结构为
{SCHEMA[task['role']]}
每个结论带 order_id、policy_section 或 review_id 作为引用。"""
    resp = client.chat.completions.create(
        model=MODEL,
        messages=[{"role": "user", "content": prompt}],
        response_format={"type": "json_object"}, extra_body=NO_THINK,
    )
    return json.loads(resp.choices[0].message.content)


async def run_worker(task: dict) -> dict:
    return await asyncio.to_thread(call_worker, task)


# ANCHOR_END: worker
# ANCHOR: synth
def synthesize(tasks: list[dict], results: list[dict]) -> str:
    bundle = [{"task": t, "result": r} for t, r in zip(tasks, results)]
    prompt = f"""你是汇总者。根据任务和结果写一条给顾客的回复草稿，不超过 180 字。
每个结论后面必须带引用：订单事实用 [order:A1659]，政策条款用 [policy:条款标题]，评论用 [review:评论 id]。
不要写工作者没查到的内容；不能承诺政策不允许的事。
任务和结果：{json.dumps(bundle, ensure_ascii=False)}"""
    resp = client.chat.completions.create(
        model=MODEL, messages=[{"role": "user", "content": prompt}], max_tokens=400,
        extra_body=NO_THINK)
    return resp.choices[0].message.content.strip()


# ANCHOR_END: synth
async def main() -> None:
    ticket = "顾客 C082 说订单 A1659 的《大运河两千年》（书号 B12）缺了二十多页，问怎么处理。"
    plan = plan_for(ticket)
    tasks = plan["tasks"]
    print(f"计划：{' / '.join(t['id'] + ' ' + t['role'] for t in tasks)}")
    print(f"任务覆盖：{' → '.join(t['role'] for t in tasks)}")
    for t in tasks:
        print(f"  {t['id']} 输入：goal={t['goal']!r} inputs={json.dumps(t['inputs'], ensure_ascii=False)}")

    results = await asyncio.gather(*(run_worker(t) for t in tasks))
    for t, r in zip(tasks, results):
        body = json.dumps(r, ensure_ascii=False)
        print(f"  {t['id']} {t['role']} 输出：{body[:150]}{'…' if len(body) > 150 else ''}")

    draft = synthesize(tasks, results)
    print("汇总草稿：" + draft)


if __name__ == "__main__":
    asyncio.run(main())
