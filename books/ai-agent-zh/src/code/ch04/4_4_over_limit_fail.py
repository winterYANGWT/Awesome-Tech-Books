"""跳过校验，直接退款：业务规则在数据层把它拦下。"""
# mode: fail
# llm: none
# expect: 超过 500
from shop import Store

store = Store()
print("直接退款 680 元，不经过 Pydantic，也不问主管")
print(store.refund("A1680", 680, "装订脱落"))
