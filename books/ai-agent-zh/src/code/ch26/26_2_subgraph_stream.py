# 26.2 子图的检查点和流：namespace 是 tuple，父图是 ()。
# mode: run
# llm: none
# expect: policy_check
# expect: \(父图\)

from typing import TypedDict

from langgraph.checkpoint.memory import InMemorySaver
from langgraph.graph import END, START, StateGraph


class SubState(TypedDict):
    order: str
    detail: str
    verdict: str


class ParentState(TypedDict):
    order: str
    verdict: str


def lookup(state: SubState) -> dict:
    return {"detail": f"{state['order']} 的政策原文"}


def check(state: SubState) -> dict:
    return {"verdict": "通过"}


sub_builder = StateGraph(SubState)
sub_builder.add_node("lookup", lookup)
sub_builder.add_node("check", check)
sub_builder.add_edge(START, "lookup")
sub_builder.add_edge("lookup", "check")
sub = sub_builder.compile()

builder = StateGraph(ParentState)
# 同一个编译后的子图实例复用两次；namespace 按父图节点名区分。
builder.add_node("policy_check", sub)
builder.add_node("privacy_check", sub)
builder.add_edge(START, "policy_check")
builder.add_edge("policy_check", "privacy_check")
builder.add_edge("privacy_check", END)
saver = InMemorySaver()
graph = builder.compile(checkpointer=saver)


def label(namespace: tuple) -> str:
    """去掉 namespace 末尾的 UUID，只留下子图节点名。"""
    return "(父图)" if not namespace else namespace[0].split(":")[0]


def run(thread_id: str, *, subgraphs: bool) -> None:
    config = {"configurable": {"thread_id": thread_id}}
    for item in graph.stream({"order": "A1659", "verdict": ""}, config,
                             stream_mode="updates", subgraphs=subgraphs):
        namespace, chunk = item if subgraphs else ((), item)
        for node in chunk:
            print(f"  {label(namespace):12} {node}")


print("subgraphs=True（每项是 (namespace, chunk)）")
run("with-sub", subgraphs=True)
print("subgraphs=False（只剩父图节点）")
run("no-sub", subgraphs=False)

print("检查点（同一 thread 的两层 namespace）")
for ck in sorted(saver.list({"configurable": {"thread_id": "with-sub"}}),
                 key=lambda c: (c.config["configurable"]["checkpoint_ns"], c.metadata["step"])):
    ns = ck.config["configurable"]["checkpoint_ns"]
    print(f"  {label((ns,)) if ns else '(父图)':12} step={ck.metadata['step']}")
