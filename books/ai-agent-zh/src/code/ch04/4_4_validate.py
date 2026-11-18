"""Pydantic 校验失败后把错误回填，模型改用主管工具。"""
# expect: ValidationError
# expect: escalate_to_supervisor
import json
import os
from typing import Annotated, Literal

from openai import OpenAI
from pydantic import BaseModel, Field, ValidationError

from harness import tool
from shop import Store

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")

store = Store()
order = store.order("A1680")
assert order is not None

@tool
def submit_refund(
    category: Literal["物流查询", "退货退款", "修改地址", "政策咨询", "投诉建议", "其他", "越界"],
    order_id: Annotated[str, "订单号，形如 A1234"],
    amount: Annotated[float, "退款金额（元）"],
    reason: Annotated[str, "退款原因"],
) -> dict:
    """提交一笔退款决定。只提交，不执行退款。"""
    return {}


@tool
def escalate_to_supervisor(
    order_id: Annotated[str, "相关订单号"],
    note: Annotated[str, "要主管处理的事项"],
) -> dict:
    """把需要主管批准的事项提交给主管。"""
    return {}


# ANCHOR: model
class RefundDecision(BaseModel):
    category: Literal["物流查询", "退货退款", "修改地址", "政策咨询", "投诉建议", "其他", "越界"]
    order_id: str = Field(pattern=r"^A\d{4}$")
    amount: float = Field(ge=0, le=500)
    reason: str = Field(min_length=1)
# ANCHOR_END: model


SYSTEM = """你是青石书店的客服主管助理。按工单和订单信息处理退款。
先调用 submit_refund 提交你的退款决定；如果需要主管批准，再改用 escalate_to_supervisor。
工具只记录决定，不真正退款。"""

USER = (f"工单 T13：订单 A1680 的典藏版十二卷里有三卷装订脱落，顾客要求全额退款。"
        f"订单实付 {order['total']:.2f} 元，已签收。请提交退款决定，金额按实付金额写。")


def call(messages: list, choice) -> tuple[str, dict]:
    resp = client.chat.completions.create(
        model=MODEL,
        messages=messages,
        tools=[submit_refund.schema(), escalate_to_supervisor.schema()],
        tool_choice=choice,
        max_tokens=600,
        extra_body={"thinking": {"type": "disabled"}},
    )
    msg = resp.choices[0].message
    if not msg.tool_calls:
        raise RuntimeError(f"模型没有调用工具：{msg.content!r}")
    call = msg.tool_calls[0]
    return call.function.name, json.loads(call.function.arguments)


# ANCHOR: loop
def main() -> None:
    messages = [{"role": "system", "content": SYSTEM}, {"role": "user", "content": USER}]
    name, args = call(messages, {"type": "function", "function": {"name": "submit_refund"}})
    print(f"第 1 次 {name}  amount={args.get('amount')}")

    try:
        decision = RefundDecision.model_validate(args)
    except ValidationError as e:
        err = e.errors()[0]
        path = ".".join(str(x) for x in err["loc"])
        print(f"ValidationError：字段路径 {path}；{err['msg']}（输入 {err['input']}）")
        feedback = (f"校验失败：{path} {err['msg']}。单笔退款超过 500 元需要主管批准，"
                    f"不要再调用 submit_refund；请改用 escalate_to_supervisor，写清楚金额和原因。")
        messages += [{"role": "assistant", "content": json.dumps(args, ensure_ascii=False)},
                     {"role": "user", "content": feedback}]
    else:
        print(f"通过校验：{decision}")
        return

    name, args = call(messages, "required")
    print(f"第 2 次 {name}  order_id={args.get('order_id')}")
    if name == "escalate_to_supervisor":
        out = store.escalate(args["order_id"], args["note"])
        print(f"业务动作：{out['status']}，未退款")
    else:
        print(f"第 2 次仍调用 {name}，转人工")
# ANCHOR_END: loop


if __name__ == "__main__":
    main()
