"""进程 2：重新打开 state.db，从同一个 thread 恢复。"""
import pathlib

from langgraph.checkpoint.sqlite import SqliteSaver
from langgraph.types import Command
from shop import Store

from ch25._lib.refund_graph import build

DB = pathlib.Path(__file__).resolve().parent / "state.db"
CONFIG = {"configurable": {"thread_id": "T13-restart"}}

with SqliteSaver.from_conn_string(str(DB)) as saver:
    graph = build(Store(), with_report=False, checkpointer=saver)
    snap = graph.get_state(CONFIG)
    print(f"进程 2：读到 next={snap.next}，amount={snap.values['amount']:.1f}")
    out = graph.invoke(Command(resume={"approved": True, "by": "主管"}), CONFIG)
    print(f"进程 2：approved={out['approved']}，{out['outcome']}")
