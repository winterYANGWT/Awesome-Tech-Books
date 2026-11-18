"""第 17 章共用：单 agent 基线、三个专员、manager 包装和 handoff 循环。

示例只改这里的提示和工具子集，不改 `shop` 和 `harness`。
"""
from __future__ import annotations

import json
from typing import Annotated

from harness import MODEL, Tool, Toolbox, client, run, short, tool
from shop.tools import SUPPORT_PROMPT, support_tools

NO_THINK = {"thinking": {"type": "disabled"}}
MAX_TOKENS = 400

# ANCHOR: single
SINGLE_PROMPT = SUPPORT_PROMPT + """
按下面的 if-then 规则处理，不要跳步：
- 顾客问订单到哪了、什么时候到：先 get_order，回答快递公司和签收日期。
- 顾客说箱子压坏、书破损、缺页、装订脱落：这是质量问题，直接 issue_refund 退全款，不用再问。
- 顾客说不想要了、买重复了：先 get_order 看实付，再 issue_refund 退书款。
- 顾客问能不能退、退货期限：用 search_policies 查“退货期限”。
- 顾客问会员、积分、发票、配送规则：用 search_policies 查。
- 顾客要改地址：用 update_address。
- 退款超过 500 元、投诉、拿不准：用 escalate_to_supervisor。
- 其他问题：用 search_books 或 list_reviews。
回答用中文，简洁，直接给结果。"""
# ANCHOR_END: single


def single_once(store, question: str, *, max_turns: int = 4) -> dict:
    """一个 agent、九个工具，跑一次工单。"""
    res = run([{"role": "system", "content": SINGLE_PROMPT},
               {"role": "user", "content": question}],
              support_tools(store), max_turns=max_turns, verbose=False,
              extra_body=NO_THINK, max_tokens=300)
    return {"answer": res.answer or "", "calls": res.turns, "usage": dict(res.usage),
            "steps": res.steps, "stop": res.stop}


LABELS = {"triage": "分诊", "logistics": "物流专员", "refund": "退款专员", "policy": "政策专员"}

PROMPTS = {
    "triage": """你是青石书店的分诊专员。你只判断该由哪位专员接手，不查订单、不回答业务问题。
- 每一轮都必须调用一个 transfer_to_* 工具，不能只输出文字；也不要输出“已转回分诊”这类回答。
- 调用 transfer_to_* 时，content 先用中文写一句“我帮您转接给××专员”，再调用工具。
- 每次只转一个专员，转接原因写一句话。转接后由接手的专员直接回答顾客，你不再参与。""",
    "logistics": """你是青石书店的物流专员。你只看到分诊转来的问题，看不到其他顾客的信息。
用 get_order 查订单的快递公司、签收日期、物流状态和是否破损；顾客要改地址时用 update_address（只有待发货能改）。
不要办理退款，不要回答会员、积分、发票问题。
输出不超过 100 字的摘要：第一行写“订单 A1234”，再写快递、签收日期和结论。""",
    "refund": """你是青石书店的退款专员。你只看到分诊转来的问题，看不到其他顾客的信息。
先 get_order 核对订单状态和实付金额，再用 search_policies 查退货政策，算出应退金额。
质量问题（缺页、破损、装订脱落）签收后 60 天内退全款；非质量问题 15 天内只退书款；超过 500 元用 escalate_to_supervisor。
需要退款就调用 issue_refund，不要只给建议。
输出不超过 120 字的摘要，必须包含订单号、退款金额（元）和依据。""",
    "policy": """你是青石书店的政策专员。你只看到分诊转来的问题，看不到其他顾客的信息。
用 search_policies 查会员、积分、发票、配送规则；引用小节标题和条款，不要凭记忆。
输出不超过 100 字的摘要，写清适用的小节和结论。""",
}

# 每个域的工具子集不超过 4 个
INCLUDE = {
    "logistics": ("get_order", "update_address"),
    "refund": ("get_order", "search_policies", "issue_refund", "escalate_to_supervisor"),
    "policy": ("search_policies",),
}


def transfer_tool(target: str, when: str) -> Tool:
    """handoff 就是一个普通的工具：模型调用它，循环换掉系统提示和工具子集。"""
    def fn(reason: str = "") -> str:
        return f"已转接给{LABELS[target]}"

    return Tool(name=f"transfer_to_{target}",
                description=f"什么时候转：{when} 转接后由{LABELS[target]}直接回答，你不再参与。",
                parameters={"type": "object",
                            "properties": {"reason": {"type": "string",
                                                      "description": "一句话转接原因，写进转接记录"}},
                            "required": []},
                fn=fn)


HANDOFF_WHEN = {
    "logistics": "顾客问某张订单的快递公司、签收日期、配送进度、破损，或要修改收货地址。",
    "refund": "顾客要求退款、问能退多少、退货进度、质量问题索赔，或退款超过 500 元要审批。",
    "policy": "顾客问会员、积分、发票、某个地区用哪家快递、能否指定快递、退货期限。",
}


def toolbox_for(store, domain: str, *, back: bool = False) -> Toolbox:
    if domain == "refund" and back:
        # 失败版本：故意不给 search_policies，只留订单、退款和转回分诊
        box = support_tools(store, include=("get_order", "issue_refund", "escalate_to_supervisor"))
        box.add(transfer_tool("triage", "你缺政策工具，或遇到自己处理不了的问题时，转回分诊。"))
        return box
    return support_tools(store, include=INCLUDE[domain])


def ask_specialist(store, domain: str, question: str, *, max_turns: int = 5) -> object:
    return run([{"role": "system", "content": PROMPTS[domain]},
                {"role": "user", "content": question}],
               toolbox_for(store, domain), max_turns=max_turns, verbose=False,
               extra_body=NO_THINK, max_tokens=MAX_TOKENS)


def add_usage(total: dict, usage) -> None:
    for k, v in (usage or {}).items():
        if isinstance(v, (int, float)):
            total[k] = round(total.get(k, 0) + v, 6)


# ANCHOR: manager
MANAGER_PROMPT = """你是青石书店的总客服，直接对顾客说话。你没有业务工具，只能把问题交给专员。
每个顾客问题只问一个最相关的专员，不要重复问同一个专员。
问题里带上订单号和顾客原话；专员返回的是内部摘要，不要原样贴给顾客。
只有你能对顾客说话，专员不对顾客说话。"""


def manager_once(store, question: str, *, max_turns: int = 4) -> dict:
    """manager 持有对话；三个专员是三个工具，内部各跑一次 harness.run。"""
    log: list[dict] = []

    def ask(domain: str, q: str) -> str:
        res = ask_specialist(store, domain, q)
        log.append({"domain": domain, "question": q, "answer": res.answer or "",
                    "turns": res.turns, "usage": res.usage})
        return res.answer or "（专员没有给出摘要）"

    @tool
    def ask_logistics(question: Annotated[str, "要问物流专员的话，带上订单号"]) -> str:
        """查某张订单的物流：快递公司、签收日期、配送进度、是否破损，以及修改收货地址。
        问题里带上订单号。配送规则、某个地区用哪家快递不要问它。"""
        return ask("logistics", question)

    @tool
    def ask_refund(question: Annotated[str, "要问退款专员的话，带上订单号和顾客原话"]) -> str:
        """处理退款金额、退货进度、质量问题退全款、超过 500 元的主管审批。
        问题里带上订单号和顾客原话。"""
        return ask("refund", question)

    @tool
    def ask_policy(question: Annotated[str, "要问政策专员的话，写清地区和问题"]) -> str:
        """查店铺政策：会员积分、发票、某个地区用哪家快递、能不能指定快递、退货期限。
        没有订单号的政策问题问它。"""
        return ask("policy", question)

    res = run([{"role": "system", "content": MANAGER_PROMPT},
               {"role": "user", "content": question}],
              Toolbox([ask_logistics, ask_refund, ask_policy]),
              max_turns=max_turns, verbose=False, extra_body=NO_THINK, max_tokens=MAX_TOKENS)
    usage = dict(res.usage)
    for item in log:
        add_usage(usage, item["usage"])
    return {"answer": res.answer or "", "log": log, "usage": usage,
            "calls": res.turns + sum(item["turns"] for item in log),
            "speakers": ["manager"]}
# ANCHOR_END: manager


class HandoffLoop(Exception):
    def __init__(self, message: str, trace: list[str]):
        super().__init__(message)
        self.trace = list(trace)


# ANCHOR: handoff
def handoff_once(store, question: str, *, max_handoffs: int = 3, allow_back: bool = False,
                 max_turns: int = 8) -> dict:
    """分诊先把对话交出去；接手 agent 直接回答，原 agent 不再参与。"""
    current, transfers, speakers, trace, calls, usage = "triage", [], [], [], 0, {}
    boxes: dict[str, Toolbox] = {}

    def prompt_for(domain: str) -> str:
        text = PROMPTS[domain]
        if allow_back and domain == "refund":
            text += ("\n注意：你手里没有 search_policies。任何退款都要先核对退货政策；"
                     "缺政策条款时调用 transfer_to_triage 转回分诊，不要凭记忆或直接退款。")
        if allow_back and domain == "triage":
            text += ("\n注意：收到退款专员要政策条款的请求时，只要带订单号或是退款问题，"
                     "仍然转回退款专员；不要自己回答，也不要转给政策专员。")
        return text

    messages = [{"role": "system", "content": prompt_for("triage")},
                {"role": "user", "content": question}]

    def box_for(domain: str) -> Toolbox:
        if domain == "triage":
            return Toolbox([transfer_tool(name, when) for name, when in HANDOFF_WHEN.items()])
        if domain not in boxes:
            boxes[domain] = toolbox_for(store, domain, back=allow_back)
        return boxes[domain]

    for _ in range(max_turns):
        box = box_for(current)
        kwargs = dict(model=MODEL, messages=messages, tools=box.schemas(),
                      max_tokens=MAX_TOKENS, extra_body=NO_THINK)
        # ANCHOR: hardroute
        if allow_back and current == "triage":
            # 失败版本把分诊写成硬路由：看到退款就转退款，不给它自由回答的机会
            kwargs["tool_choice"] = {"type": "function",
                                     "function": {"name": "transfer_to_refund"}}
        if allow_back and current == "refund":
            # 退款专员也写成硬规则：没有政策工具就先转回分诊，不直接退款
            kwargs["tool_choice"] = {"type": "function",
                                     "function": {"name": "transfer_to_triage"}}
        # ANCHOR_END: hardroute
        resp = client().chat.completions.create(**kwargs)
        calls += 1
        add_usage(usage, resp.usage.model_dump() if resp.usage else {})
        msg = resp.choices[0].message
        content = (msg.content or "").strip()
        entry = {"role": "assistant", "content": msg.content or ""}
        if msg.tool_calls:
            entry["tool_calls"] = [tc.model_dump(include={"id", "type", "function"})
                                   for tc in msg.tool_calls]
        messages.append(entry)
        if content:
            trace.append(f"  {LABELS[current]}：{short(content, 90)}")
            if current not in speakers:
                speakers.append(current)
        if not msg.tool_calls:
            return {"answer": content, "final": current, "transfers": transfers,
                    "speakers": speakers, "trace": trace, "calls": calls, "usage": usage,
                    "messages": messages}
        for tc in msg.tool_calls:
            name = tc.function.name
            try:
                args = json.loads(tc.function.arguments or "{}")
            except json.JSONDecodeError:
                args = {}
            if allow_back and name.startswith("transfer_to_"):
                # 硬路由：不信任模型返回的转接目标，按当前 agent 强制指定下一站
                name = "transfer_to_refund" if current == "triage" else "transfer_to_triage"
            # ANCHOR: transfer
            if name.startswith("transfer_to_") and name.removeprefix("transfer_to_") in LABELS:
                target = name.removeprefix("transfer_to_")
                reason = str(args.get("reason", ""))
                if len(transfers) >= max_handoffs:
                    trace.append(f"  第 {len(transfers) + 1} 次转接被拒绝："
                                 f"{LABELS[current]} → {LABELS[target]}（max_handoffs={max_handoffs}）")
                    raise HandoffLoop(f"第 {len(transfers) + 1} 次转接被拒绝，"
                                      f"超过 max_handoffs={max_handoffs}", trace)
                trace.append(f"  {LABELS[current]} → {name}(reason={reason!r})")
                # 先补一条 role: tool 的“已转接”结果，再换系统提示和工具子集
                messages.append({"role": "tool", "tool_call_id": tc.id,
                                 "content": f"已转接给{LABELS[target]}。接下来的对话由"
                                            f"{LABELS[target]}直接回答顾客。"})
                transfers.append({"from": current, "to": target, "reason": reason})
                trace.append(f"  已转接：{LABELS[target]}")
                if current not in speakers:
                    speakers.append(current)
                current = target
                messages[0] = {"role": "system", "content": prompt_for(target)}
                box = box_for(current)
            # ANCHOR_END: transfer
            else:
                out = box.call(name, tc.function.arguments)
                messages.append({"role": "tool", "tool_call_id": tc.id, "content": out})
                trace.append(f"  {LABELS[current]} → {name}("
                             f"{short(json.dumps(args, ensure_ascii=False), 40)})  ← {short(out, 50)}")
    raise RuntimeError(f"超过 {max_turns} 轮还没有回答")
# ANCHOR_END: handoff
