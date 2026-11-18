# 25.1 两个 thread：A 记得上一句问的是哪张订单，B 是空状态。
# mode: run
# expect: 82
# expect: 订单号
"""给图装上 InMemorySaver，同一个 thread_id 才有跨轮记忆。"""
import os
from typing import Annotated, TypedDict

from langchain_core.messages import AnyMessage, HumanMessage, SystemMessage
from langchain_core.tools import StructuredTool
from langchain_openai import ChatOpenAI
from langgraph.checkpoint.memory import InMemorySaver
from langgraph.graph import START, StateGraph
from langgraph.graph.message import add_messages
from langgraph.prebuilt import ToolNode, tools_condition

from shop import TODAY, Store
from shop.tools import support_tools

MODEL = os.environ.get("MODEL", "deepseek-flash")
store = Store()
box = support_tools(store, include=("get_order", "search_policies"))


def lc_tool(name: str) -> StructuredTool:
    t = box.tools[name]
    return StructuredTool.from_function(func=t.fn, name=t.name, description=t.description)


tools = [lc_tool("get_order"), lc_tool("search_policies")]
model = ChatOpenAI(model=MODEL).bind_tools(tools)
SYSTEM = f"""你是青石书店的客服。今天是 {TODAY}。
先用工具查订单和政策再回答；顾客没给订单号时先问是哪张订单，不要猜。
质量问题退全款（书款加运费），签收 60 天内可以申请。回答简洁，给出金额。"""


class State(TypedDict):
    messages: Annotated[list[AnyMessage], add_messages]


def assistant(state: State):
    reply = model.invoke([SystemMessage(SYSTEM)] + state["messages"])
    return {"messages": [reply]}


builder = StateGraph(State)
builder.add_node("assistant", assistant)
builder.add_node("tools", ToolNode(tools))
builder.add_edge(START, "assistant")
builder.add_conditional_edges("assistant", tools_condition)
builder.add_edge("tools", "assistant")
graph = builder.compile(checkpointer=InMemorySaver())

T06 = "收到的《大运河两千年》中间缺了二十多页，订单号 A1659，怎么处理？"


def ask(thread_id: str, text: str):
    config = {"configurable": {"thread_id": thread_id}}
    out = graph.invoke({"messages": [HumanMessage(text)]}, config)
    return config, out


config_a, out_a = ask("T06-A", T06)
config_a, out_a = ask("T06-A", "那能退多少？")
config_b, out_b = ask("T06-B", "那能退多少？")

print(f"线程 A（T06-A）消息 {len(out_a['messages'])} 条，检查点 {len(list(graph.get_state_history(config_a)))} 个")
print(f"A 的回答：{out_a['messages'][-1].content}")
print(f"线程 B（T06-B）消息 {len(out_b['messages'])} 条，检查点 {len(list(graph.get_state_history(config_b)))} 个")
print(f"B 的回答：{out_b['messages'][-1].content}")
