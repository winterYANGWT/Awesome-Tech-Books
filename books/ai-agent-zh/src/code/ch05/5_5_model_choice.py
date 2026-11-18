# mode: run
# expect: deepseek-v4-pro
"""T13 的 680 元退款：同一提示、同一对工具，只换模型。"""
import json
import os
import sys
import pathlib

from openai import OpenAI

client = OpenAI()                                  # 读 OPENAI_BASE_URL、OPENAI_API_KEY
MODEL = os.environ.get("MODEL", "deepseek-flash")

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from ch05.common import Store, order_brief, order_id_of
from shop import ShopError, tickets

T13 = next(t for t in tickets() if t["id"] == "T13")
store = Store()
order = store.order(order_id_of(T13["text"]))
policy = store.policy("returns")
rules = [ln for ln in policy.splitlines() if "500" in ln or "60 天" in ln][:2]

TOOLS = [
    {"type": "function", "function": {
        "name": "issue_refund", "description": "直接给订单退款。单笔超过 500 元不能直接退。",
        "parameters": {"type": "object", "properties": {
            "order_id": {"type": "string", "description": "订单号，形如 A1680"},
            "amount": {"type": "number", "description": "退款金额（元）"}}, "required": ["order_id", "amount"]}}},
    {"type": "function", "function": {
        "name": "escalate_to_supervisor", "description": "把超过权限的退款提交给主管。",
        "parameters": {"type": "object", "properties": {
            "order_id": {"type": "string"}, "note": {"type": "string"}}, "required": ["order_id", "note"]}}},
]
SYSTEM = "你是退款客服。按政策决定：能直接退就 issue_refund，超过权限就 escalate_to_supervisor。只调用一个工具。"

# 先用 pro 建基线，再用 flash 试降级：同一提示、同一对工具，只换模型。
for model in ("deepseek-v4-pro", "deepseek-flash"):
    payload = {"ticket": T13["text"], "order": order_brief(order)}
    resp = client.chat.completions.create(
        model=model,
        messages=[{"role": "system", "content": SYSTEM + "\n政策：" + "；".join(rules)},
                  {"role": "user", "content": json.dumps(payload, ensure_ascii=False)}],
        tools=TOOLS, tool_choice="required", max_tokens=300,
        extra_body={"thinking": {"type": "disabled"}})
    call = (resp.choices[0].message.tool_calls or [None])[0]
    name = call.function.name if call else "（没有调用）"
    args = json.loads(call.function.arguments) if call else {}
    if name == "issue_refund":
        try:
            store.refund(args["order_id"], args["amount"], "T13 测试")
            result, escalated = "退款成功", "否"
        except ShopError as e:
            result, escalated = f"工具拒绝：{e}", "否"
    elif name == "escalate_to_supervisor":
        store.escalate(args.get("order_id", ""), args.get("note", ""))
        result, escalated = "已提交主管", "是"
    else:
        result, escalated = "没有动作", "否"
    u = resp.usage
    print(f"[{model}] 选择 {name}({json.dumps(args, ensure_ascii=False)})")
    print(f"            结果：{result}；是否转主管：{escalated}")
    print(f"            usage：prompt {u.prompt_tokens}，completion {u.completion_tokens}")

print("实际动作：", [(a["kind"], a["order_id"], a["detail"]) for a in store.actions()])
