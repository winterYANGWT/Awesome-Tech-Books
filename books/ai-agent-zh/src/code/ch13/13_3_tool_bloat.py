# mode: run
# timeout: 480
# expect: 27
"""9 个职责清晰的工具，对比 27 个（其中 18 个是转调同一方法的别名）。"""
import os

from openai import OpenAI

from harness import Tool, Toolbox, run
from shop import Store, tickets
from shop.tools import SUPPORT_PROMPT, support_tools

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")
IDS = ["T06", "T09", "T12", "T15", "T19", "T24", "T13", "T22"]
ALL = {t["id"]: t for t in tickets()}
CANON = {"find_customer", "get_customer_context", "get_order", "search_books", "search_policies",
         "list_reviews", "issue_refund", "update_address", "escalate_to_supervisor"}
ALIASES = {
    "get_order": [("lookup_order", "查询订单详情、金额和状态"), ("find_order", "查找订单"),
                  ("order_detail", "订单的明细信息"), ("fetch_order", "获取一张订单"),
                  ("query_order", "查询订单信息")],
    "find_customer": [("lookup_customer", "查找顾客"), ("customer_search", "搜索顾客资料")],
    "get_customer_context": [("customer_overview", "顾客概况和最近订单")],
    "search_policies": [("lookup_policy", "查询政策条款"), ("find_policy", "查找政策"),
                        ("policy_search", "搜索店铺政策")],
    "list_reviews": [("get_reviews", "读取顾客评论")],
    "issue_refund": [("refund_order", "给订单退款"), ("process_refund", "处理退款，包括超过 500 元的审批")],
    "update_address": [("change_address", "修改收货地址"), ("edit_address", "编辑订单地址")],
    "escalate_to_supervisor": [("escalate_case", "把事项提交主管"), ("supervisor_review", "提交主管复核")],
}


def judge(t, answer, actions):
    exp = t["expect"]
    refunds = [a for a in actions if a["kind"] == "refund"]
    adds = [a for a in actions if a["kind"] == "update_address"]
    esc = [a for a in actions if a["kind"] == "escalate"]
    if exp["refund"] is None:
        if refunds:
            return False
    elif not any(a["order_id"] == exp["refund"]["order_id"] and abs(a["detail"]["amount"] - exp["refund"]["amount"]) < 0.01
                 for a in refunds):
        return False
    if exp["address_changed"] is None:
        if adds:
            return False
    elif not any(a["order_id"] == exp["address_changed"]["order_id"] and exp["address_changed"]["contains"] in a["detail"]["new"]
                 for a in adds):
        return False
    if exp.get("escalate") and not esc:
        return False
    if any(w not in answer for w in exp["mention"]):
        return False
    if any(w in answer for w in exp.get("must_not_mention", [])):
        return False
    return True


def toolbox(store, bloat):
    base = support_tools(store)
    tools = list(base.tools.values())
    if not bloat:
        return Toolbox(tools)
    extra = []
    for name, pairs in ALIASES.items():
        source = base.tools[name]
        for alias, desc in pairs:
            text = ("处理退款，单笔超过 500 元时提交主管审批" if alias == "process_refund" else
                    "提交主管复核退款或投诉" if alias == "supervisor_review" else source.description)
            extra.append(Tool(name=alias, description=text, parameters=source.parameters, fn=source.fn))
    return Toolbox(extra + tools)


print("工具数  别名/混用   总工具调用   平均轮数   每轮 prompt   每工单累计")
for bloat in (False, True):
    good, alias_calls, mixed, turns, calls, tokens = 0, 0, 0, 0, 0, 0
    for tid in IDS:
        store = Store()
        box = toolbox(store, bloat)
        res = run([{"role": "system", "content": SUPPORT_PROMPT},
                   {"role": "user", "content": ALL[tid]["text"]}], box, model=MODEL, max_turns=6, verbose=False,
                  extra_body={"thinking": {"type": "disabled"}})
        alias_calls += sum(1 for s in res.steps if s.name not in CANON)
        calls += len(res.steps)
        tokens += res.usage.get("prompt_tokens", 0)
        if tid == "T13" and any("refund" in s.name for s in res.steps):
            mixed += 1
        turns += res.turns
    print(f"{len(box.tools):<7}{alias_calls + mixed:>10}{calls:>12}{turns / len(IDS):>10.1f}"
          f"{round(tokens / turns):>14,}{round(tokens / len(IDS)):>14,}")
