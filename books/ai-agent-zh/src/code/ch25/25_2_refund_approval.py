# 25.2 主管批准 680 元退款：图在 approve 节点中断，另一次 invoke 恢复。
# mode: run
# expect: 680.0
# expect: approved_by.*主管
"""有 InMemorySaver 才能恢复；`Command(resume=...)` 的值成为 interrupt() 的返回值。"""
import json

from langgraph.checkpoint.memory import InMemorySaver
from langgraph.types import Command

from ch25._lib.refund_graph import build
from shop import Store

store = Store()
graph = build(store, checkpointer=InMemorySaver())
config = {"configurable": {"thread_id": "T13-approval"}}

# 第一次 invoke：propose 算出 680，approve 调 interrupt() 停下来。
paused = graph.invoke({"order_id": "A1680", "reason": "三卷装订脱落", "amount": 0.0,
                       "approved": False, "by": "", "note": "", "edited_by": "", "edit_note": "",
                       "outcome": {}, "reply": ""}, config)
pending = paused["__interrupt__"][0]
print(f"中断 payload：{json.dumps(pending.value, ensure_ascii=False)}")
print(f"停下时 next={graph.get_state(config).next}")

# 主管批准；resume 的值就是 interrupt() 的返回值。
final = graph.invoke(Command(resume={"approved": True, "by": "主管"}), config)
print(f"退款结果：{final['outcome']}")
for action in store.actions():
    d = action["detail"]
    print(f"action：{action['kind']} {d['amount']:.2f} approved_by={d['approved_by']}")
print(f"回复：{final['reply']}")
