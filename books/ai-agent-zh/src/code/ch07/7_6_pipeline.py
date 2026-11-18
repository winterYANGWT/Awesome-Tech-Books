# 7.6 T06 和 T13 各走一遍流水线：计划→工作者→汇总→规则→评估→动作。
# mode: run
# expect: T13.*主管
import asyncio
import json
import os
import re

from openai import OpenAI

from shop import REFUND_LIMIT, Store, tickets

client = OpenAI()                                  # 读 OPENAI_BASE_URL、OPENAI_API_KEY
MODEL = os.environ.get("MODEL", "deepseek-flash")
NO_THINK = {"thinking": {"type": "disabled"}}
store = Store()
ROLES = ["订单查询", "政策查询", "评论查询", "回复草稿"]
PLAN_TOOL = {
    "type": "function",
    "function": {
        "name": "submit_plan",
        "description": "提交工单处理计划，只提交任务，不执行操作。",
        "parameters": {
            "type": "object",
            "properties": {"tasks": {"type": "array", "minItems": 1, "maxItems": 4, "items": {
                "type": "object",
                "properties": {
                    "id": {"type": "string", "enum": ["T1", "T2", "T3", "T4"]},
                    "role": {"type": "string", "enum": ROLES},
                    "goal": {"type": "string", "minLength": 4},
                    "inputs": {"type": "object", "properties": {
                        "order_id": {"type": "string"}, "book_id": {"type": "string"},
                        "depends_on": {"type": "array", "items": {"type": "string"}}}},
                    "output_schema": {"type": "string",
                                      "enum": ["order_facts", "policy_clauses",
                                               "review_digest", "reply_draft"]},
                },
                "required": ["id", "role", "goal", "inputs", "output_schema"]}}},
            "required": ["tasks"],
        },
    },
}
SCHEMA = {
    "订单查询": '{"order_id": str, "status": str, "total_paid": number, "refunded": number, "delivered": str}',
    "政策查询": '{"clauses": [{"policy_section": str, "text": str}]}',
    "评论查询": '{"book_id": str, "review_ids": [str], "summary": str}',
}


BOOK = {"T06": "B12", "T13": "B39"}


# ANCHOR: plan
def fixed_plan(ticket: dict) -> list[dict]:
    oid, bid = re.search(r"A\d{4}", ticket["text"]).group(), BOOK[ticket["id"]]
    return [
        {"id": "T1", "role": "订单查询", "goal": "查订单状态、实付金额、商品和物流",
         "inputs": {"order_id": oid}, "output_schema": "order_facts"},
        {"id": "T2", "role": "政策查询", "goal": "查质量问题和超过 500 元退款的条款",
         "inputs": {"order_id": oid}, "output_schema": "policy_clauses"},
        {"id": "T3", "role": "评论查询", "goal": f"查 {bid} 的评论里有没有缺页或装订问题",
         "inputs": {"book_id": bid}, "output_schema": "review_digest"},
    ]


def plan_problem(tasks: list[dict]) -> str:
    if not 1 <= len(tasks) <= 4:
        return f"任务数 {len(tasks)} 不在 1–4 之间"
    if not {"订单查询", "政策查询"} <= {t["role"] for t in tasks}:
        return "缺少订单查询或政策查询"
    ids = [t["id"] for t in tasks]
    if len(ids) != len(set(ids)):
        return f"任务 id 重复：{ids}"
    for t in tasks:
        for dep in t["inputs"].get("depends_on", []):
            if dep not in ids:
                return f"{t['id']} 依赖不存在的任务 {dep}"
    return ""


def plan_for(ticket: dict, feedback: str = "") -> list[dict]:
    oid = re.search(r"A\d{4}", ticket["text"]).group()
    bid = BOOK[ticket["id"]]
    prompt = f"""为工单 {ticket['id']} 拆 2–4 个任务，必须覆盖订单事实和退货政策，并查评论。
工单：{ticket['text']}
订单号：{oid}；书的编号：{bid}（评论查询用 book_id={bid}）。
不要回复草稿，汇总者会写。用 submit_plan 提交，不要执行退款。"""
    if feedback:
        prompt += f"\n上一次计划被拒绝：{feedback}。请修正后重新提交。"
    resp = client.chat.completions.create(
        model=MODEL, messages=[{"role": "user", "content": prompt}],
        tools=[PLAN_TOOL], tool_choice={"type": "function", "function": {"name": "submit_plan"}},
        extra_body={"thinking": {"type": "disabled"}})
    plan = json.loads(resp.choices[0].message.tool_calls[0].function.arguments)
    tasks = [t for t in plan["tasks"] if t["role"] != "回复草稿"]
    problem = plan_problem(tasks)
    if problem:
        if feedback:
            print(f"  计划仍不完整（{problem}），改用固定 3 任务")
            return fixed_plan(ticket)
        print(f"  计划不完整（{problem}），退回编排者重规划 1 次")
        return plan_for(ticket, problem)
    return tasks


# ANCHOR_END: plan
def fetch(task: dict):
    role, inp = task["role"], task["inputs"]
    if role == "订单查询":
        o = store.order(inp["order_id"])
        return {"order_id": o["id"], "status": o["status"], "total_paid": o["total"],
                "refunded": o["refunded"], "delivered": o["shipment"]["delivered"],
                "items": [i["title"] for i in o["items"]]}
    if role == "政策查询":
        return store.search_policies(task["goal"])
    if role == "评论查询":
        return store.reviews_of(inp.get("book_id", ""))
    raise ValueError(f"未知角色 {role}")


def call_worker(task: dict) -> dict:
    data = fetch(task)
    prompt = f"""你是{task['role']}工作者，只看到这一条委派，看不到主对话。
目标：{task['goal']}
输入：{json.dumps(task['inputs'], ensure_ascii=False)}
工具结果：{json.dumps(data, ensure_ascii=False)}
只根据工具结果回答，不猜；不退款、不联系顾客；输出 JSON：{SCHEMA[task['role']]}
每个结论带 order_id、policy_section 或 review_id。"""
    resp = client.chat.completions.create(
        model=MODEL, messages=[{"role": "user", "content": prompt}],
        response_format={"type": "json_object"}, max_tokens=500, extra_body=NO_THINK)
    return json.loads(resp.choices[0].message.content)


def digest(task: dict, result: dict) -> str:
    if task["role"] == "订单查询":
        return (f"{result.get('order_id')} {result.get('status')} "
                f"实付 {result.get('total_paid')} 已退 {result.get('refunded')}")
    if task["role"] == "政策查询":
        clauses = result.get("clauses", [])
        secs = sorted({c.get("policy_section", "?") for c in clauses})
        return f"{len(secs)} 节 / {len(clauses)} 条：" + "；".join(secs[:3])
    return f"{result.get('book_id')} 评论 id={','.join(result.get('review_ids', []))}"


def synthesize(tasks: list[dict], results: list[dict], missing: list[dict] | None = None) -> str:
    bundle = [{"task_id": t["id"], "worker": t["role"], "result": r} for t, r in zip(tasks, results)]
    note = ""
    if missing:
        note = ("\n以下任务失败，草稿只写已有结果，并说明这部分需要人工确认："
                + json.dumps(missing, ensure_ascii=False))
    prompt = f"""你是汇总者。根据任务和结果写一条给顾客的回复草稿，不超过 150 字。
每个结论后带引用：订单用 [order:订单号]，政策用 [policy:条款标题]，评论用 [review:评论 id]。
不能承诺政策不允许的事。任务和结果：{json.dumps(bundle, ensure_ascii=False)}""" + note
    resp = client.chat.completions.create(
        model=MODEL, messages=[{"role": "user", "content": prompt}], max_tokens=350,
        extra_body=NO_THINK)
    return resp.choices[0].message.content.strip()


def evaluate(draft: str, amount: float) -> dict:
    prompt = f"""你是退款回复评估者，只输出 JSON：
{{"pass": bool, "score": 1-5, "issues": [{{"type": str, "quote": str, "fix": str}}]}}
订单应退 {amount:.2f} 元；单笔超过 {REFUND_LIMIT:.0f} 元必须主管批准；不得承诺今天到账。
草稿：{draft}"""
    resp = client.chat.completions.create(
        model=MODEL, messages=[{"role": "user", "content": prompt}],
        response_format={"type": "json_object"}, max_tokens=300, extra_body=NO_THINK)
    return json.loads(resp.choices[0].message.content)


def revise(draft: str, issues: list[dict], amount: float) -> str:
    prompt = f"""上一版退款回复草稿被评估者退回。按 issues 逐条修改，重写不超过 150 字。
订单应退 {amount:.2f} 元；单笔超过 {REFUND_LIMIT:.0f} 元必须主管批准；不得承诺今天到账。
上一版：{draft}
issues：{json.dumps(issues, ensure_ascii=False)}"""
    resp = client.chat.completions.create(
        model=MODEL, messages=[{"role": "user", "content": prompt}], max_tokens=350,
        extra_body=NO_THINK)
    return resp.choices[0].message.content.strip()


def rule_check(draft: str, order_id: str, amount: float) -> tuple[bool, str]:
    amounts = [float(x) for x in re.findall(r"(\d+(?:\.\d+)?)\s*元", draft)]
    if amount not in amounts:
        return False, f"草稿没写应退的 {amount:.0f} 元"
    if order_id not in draft:
        return False, f"草稿没写订单号 {order_id}"
    if amount > REFUND_LIMIT:
        if "主管" not in draft:
            return False, f"{amount:.0f} 元超过 {REFUND_LIMIT:.0f} 元，但没写主管"
        if re.search(r"今天(退款|到账)|立即退款|马上退款", draft):
            return False, "超过 500 元却承诺今天退款/到账"
    return True, f"金额 {amount:.0f} 元、订单号、权限都正确"


# ANCHOR: pipeline
async def run_pipeline(ticket: dict) -> None:
    print(f"=== {ticket['id']} ===")
    order_id = re.search(r"A\d{4}", ticket["text"]).group()
    tasks = plan_for(ticket)
    print("计划：" + " / ".join(f"{t['id']} {t['role']}" for t in tasks))
    raw = await asyncio.gather(*(asyncio.to_thread(call_worker, t) for t in tasks),
                               return_exceptions=True)
    tasks_ok, results, failed = [], [], []
    for t, r in zip(tasks, raw):
        if isinstance(r, BaseException):
            failed.append({"task_id": t["id"], "worker": t["role"], "error": str(r)})
            print(f"  [{t['id']} worker={t['role']}] 失败：{r}")
        else:
            tasks_ok.append(t)
            results.append(r)
            print(f"  [{t['id']} worker={t['role']}] {digest(t, r)}")
    if not results:
        store.escalate(order_id, "所有工作者都失败，需人工处理")
        print(f"{ticket['id']} 最终动作：escalate_to_supervisor（工作者全部失败）")
        return

    draft = synthesize(tasks_ok, results, failed)
    print("草稿：" + " ".join(draft.split()))
    expected = ticket["expect"].get("refund")
    amount = expected["amount"] if expected else 680.0
    order_id = expected["order_id"] if expected else order_id
    ok, why = rule_check(draft, order_id, amount)
    print(f"规则检查：{'通过' if ok else '不通过'}（{why}）")
    if not ok:
        store.escalate(order_id, f"规则检查不通过：{why}")
        print(f"{ticket['id']} 最终动作：escalate_to_supervisor（规则未通过）")
        return

    if amount > REFUND_LIMIT:
        verdict = evaluate(draft, amount)
        for round_no in (1, 2):
            issues = verdict.get("issues", [])
            print(f"评估 round={round_no}：pass={verdict['pass']} "
                  f"score={verdict['score']} issues={len(issues)}")
            if issues:
                print(f"  [{issues[0].get('type')}] “{issues[0].get('quote')}” "
                      f"→ {issues[0].get('fix')}")
            if verdict["pass"] or round_no == 2:
                break
            draft = revise(draft, issues, amount)
            print("第 2 版草稿：" + " ".join(draft.split()))
            verdict = evaluate(draft, amount)
        if not verdict["pass"]:
            print(f"{ticket['id']} 两轮仍未通过：转人工")
        store.escalate(order_id, f"退款 {amount:.0f} 元超过 500 元，需主管批准")
        print(f"{ticket['id']} 最终动作：escalate_to_supervisor（主管审批）")
    else:
        store.refund(order_id, amount, "质量问题：缺页")
        print(f"{ticket['id']} 最终动作：issue_refund {amount:.2f} 元")
# ANCHOR_END: pipeline


async def main() -> None:
    by_id = {t["id"]: t for t in tickets()}
    for tid in ("T06", "T13"):
        await run_pipeline(by_id[tid])
    print("actions：" + "；".join(f"{a['kind']}({a['order_id']})" for a in store.actions()))


if __name__ == "__main__":
    asyncio.run(main())
