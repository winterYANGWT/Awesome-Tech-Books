# 25.3 中断期间人把 680 改成 600：update_state 追加检查点，不覆盖原提案。
# mode: run
# llm: none
# expect: 执行 600
"""`as_node="propose"` 让这次更新算 propose 写的；`update_state` 返回的新 config 用于恢复。"""
from langgraph.checkpoint.memory import InMemorySaver
from langgraph.types import Command

from ch25._lib.refund_graph import build
from shop import Store

store = Store()
graph = build(store, with_report=False, checkpointer=InMemorySaver())
thread_config = {"configurable": {"thread_id": "T13-edit"}}
config = thread_config

paused = graph.invoke({"order_id": "A1680", "reason": "三卷装订脱落", "amount": 0.0,
                       "approved": False, "by": "", "note": "", "edited_by": "", "edit_note": "",
                       "outcome": {}, "reply": ""}, config)
print(f"原始中断 payload：{paused['__interrupt__'][0].value['amount']:.1f} 元")
print(f"中断时的 state：amount={graph.get_state(config).values['amount']:.1f} "
      f"next={graph.get_state(config).next}")

# 主管把金额改成 600：这次更新算 propose 写的，相当于重写提案，并留下改的人和理由。
config = graph.update_state(config, {"amount": 600.0, "edited_by": "主管",
                                     "edit_note": "按主管意见改金额"}, as_node="propose")
snap = graph.get_state(config)
print(f"人改后的 state：amount={snap.values['amount']:.1f} next={snap.next}")
print(f"改的人和理由：{snap.values['edited_by']}，{snap.values['edit_note']}")

final = graph.invoke(Command(resume=True), config)
print(f"执行 {final['amount']:.1f} 元 → {final['outcome']['status']}")
print(f"actions：{[(a['kind'], a['detail']['amount']) for a in store.actions()]}")
old = [s.values["amount"] for s in graph.get_state_history(thread_config) if "amount" in s.values]
print(f"历史里的 amount 序列（新→旧）：{[round(x, 1) for x in old]}")
