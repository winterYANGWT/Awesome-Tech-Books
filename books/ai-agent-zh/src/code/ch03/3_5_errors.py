# 3.5 工具错误都变成给模型看的文字：五段
# llm: none
# expect: 没有名为
# expect: 500 元
# expect: RuntimeError
# expect: 订单 123 不存在
import json

from harness import Tool, Toolbox
from shop import Store
from shop.tools import support_tools

box = support_tools(Store(), include=("get_order", "issue_refund", "escalate_to_supervisor"))


def broken(order_id: str) -> dict:
    raise RuntimeError("库存服务连接超时")


stock = Tool("check_stock", "查库存（示例故意让它抛异常）",
             {"type": "object", "properties": {"order_id": {"type": "string"}}, "required": ["order_id"]},
             broken)

cases = [
    ("未知工具", box, "refund_order", '{"order_id": "A1680"}'),
    ("参数不是合法 JSON", box, "get_order", "{order_id: A1680}"),
    ("缺少必填参数", box, "get_order", "{}"),
    ("工具内部的业务规则", box, "issue_refund",
     json.dumps({"order_id": "A1680", "amount": 680, "reason": "装订脱落"}, ensure_ascii=False)),
    ("工具内部的意外异常", Toolbox([stock]), "check_stock", '{"order_id": "A1680"}'),
]
for i, (label, tools, name, arguments) in enumerate(cases, 1):
    print(f"{i}. {label}：")
    print(f"   {tools.call(name, arguments)}")

print("类型不对不会触发参数错误，会直接进函数：")
print(f"   {box.call('get_order', '{\"order_id\": 123}')}")
