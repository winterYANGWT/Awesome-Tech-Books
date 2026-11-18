# 25.2 同一张图，主管拒绝：execute 不退款，改提交主管，report 用模型写回复。
# mode: run
# expect: escalate
"""拒绝也走同一条恢复路径，只是 resume 的内容不同；审批人留在 state 里。"""
from langgraph.checkpoint.memory import InMemorySaver
from langgraph.types import Command

from ch25._lib.refund_graph import build
from shop import Store

store = Store()
graph = build(store, checkpointer=InMemorySaver())
config = {"configurable": {"thread_id": "T13-reject"}}

graph.invoke({"order_id": "A1680", "reason": "三卷装订脱落", "amount": 0.0,
              "approved": False, "by": "", "note": "", "edited_by": "", "edit_note": "",
              "outcome": {}, "reply": ""}, config)
final = graph.invoke(Command(resume={"approved": False, "by": "主管",
                                    "note": "超过 500 元，先提交主管"}), config)
print(f"退款结果：{final['outcome']}")
print(f"state 里的审批人：{graph.get_state(config).values['by']}")
print(f"actions：{[(a['kind'], a['order_id'], a['detail']) for a in store.actions()]}")
print(f"回复：{final['reply']}")
