"""进程 1：跑到 interrupt 后退出，检查点写进 state.db。"""
import pathlib

from langgraph.checkpoint.sqlite import SqliteSaver
from shop import Store

from ch25._lib.refund_graph import build

DB = pathlib.Path(__file__).resolve().parent / "state.db"
CONFIG = {"configurable": {"thread_id": "T13-restart"}}

with SqliteSaver.from_conn_string(str(DB)) as saver:
    graph = build(Store(), with_report=False, checkpointer=saver)
    out = graph.invoke({"order_id": "A1680", "reason": "三卷装订脱落", "amount": 0.0,
                        "approved": False, "by": "", "note": "", "edited_by": "", "edit_note": "",
                        "outcome": {}, "reply": ""}, CONFIG)
    snap = graph.get_state(CONFIG)
    print(f"进程 1：中断 {out['__interrupt__'][0].value}，next={snap.next}，然后退出")
