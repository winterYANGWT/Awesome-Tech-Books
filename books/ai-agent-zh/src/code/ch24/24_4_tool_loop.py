# 24.4 ToolNode + tools_condition：T06 缺页退款
# mode: run
# expect: 第 2 轮 → issue_refund.*82
# expect: 最终回答
import json
import os
from typing import Annotated, TypedDict

from langchain_core.messages import AnyMessage, SystemMessage
from langchain_core.tools import StructuredTool
from langchain_openai import ChatOpenAI
from langgraph.graph import START, StateGraph
from langgraph.graph.message import add_messages
from langgraph.prebuilt import ToolNode, tools_condition

from harness import short
from shop import Store, tickets
from shop.tools import SUPPORT_PROMPT, support_tools

MODEL = os.environ.get("MODEL", "deepseek-flash")
box = support_tools(Store())


def as_langchain(t) -> StructuredTool:
    """把 harness.Tool 包成 LangChain 工具，参数 schema 原样传过去。"""
    def call(**kwargs):
        return box.call(t.name, kwargs)
    return StructuredTool(name=t.name, description=t.description,
                          args_schema=t.parameters, func=call)


tools = [as_langchain(t) for t in box.tools.values()]
model = ChatOpenAI(model=MODEL, extra_body={"thinking": {"type": "disabled"}}).bind_tools(tools)


class State(TypedDict):
    messages: Annotated[list[AnyMessage], add_messages]


def assistant(state: State) -> dict:
    # 系统提示每次现拼，不写回 state
    return {"messages": [model.invoke([SystemMessage(SUPPORT_PROMPT)] + state["messages"])]}


builder = StateGraph(State)
builder.add_node("assistant", assistant)
builder.add_node("tools", ToolNode(tools, handle_tool_errors=True))
builder.add_edge(START, "assistant")
builder.add_conditional_edges("assistant", tools_condition)
builder.add_edge("tools", "assistant")
graph = builder.compile()

ticket = next(t for t in tickets() if t["id"] == "T06")["text"]
turn = 0
answer = None
for update in graph.stream({"messages": [("user", ticket)]}, stream_mode="updates"):
    for node, data in update.items():
        if node == "assistant":
            turn += 1
            msg = data["messages"][-1]
            if msg.tool_calls:
                for tc in msg.tool_calls:
                    args = json.dumps(tc["args"], ensure_ascii=False)
                    print(f"第 {turn} 轮 → {tc['name']}({args})")
            else:
                answer = msg.content
                print(f"第 {turn} 轮 → 不再调用工具")
        else:
            for m in data["messages"]:
                print(f"          ← {m.name}: {short(m.content)}")

print()
print("最终回答：" + answer)
