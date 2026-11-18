# 9.4 反例：工具抛 KeyError，harness 只把异常类型交给模型，没有“下一步”。
# mode: fail
# llm: none
# expect: 不可执行
# expect: KeyError
"""检查工具执行失败时的错误文字：要能看出错在哪、下一步怎么做，而不是只有异常类型。"""
import sys

from harness import Toolbox, tool


# ANCHOR: broken
@tool
def get_order(order_id: str) -> dict:
    """按订单号查订单。"""
    row = {}                                  # 模拟取数失败：后端返回了空结果
    return {"order_id": row["customer"]}      # KeyError: 'customer'


box = Toolbox([get_order])
text = box.call("get_order", {"order_id": "A1670"})
# ANCHOR_END: broken

print(f"模型看到的错误：{text}")
actionable = ("订单号" in text or "下一步" in text or "核对" in text or "改用" in text)
if not actionable:
    print("错误信息不可执行：只有 KeyError")
    sys.exit(1)
print("错误信息可执行")
