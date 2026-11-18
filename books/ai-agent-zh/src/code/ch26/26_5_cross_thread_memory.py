# 26.5 跨 thread 的顾客档案：thread 1 写入，thread 2 新 thread 读出。
# mode: run
# expect: 天津远山文化

import json
import os
from typing import Annotated, TypedDict

from langchain_core.messages import HumanMessage, SystemMessage
from langchain_openai import ChatOpenAI
from langgraph.checkpoint.memory import InMemorySaver
from langgraph.graph import END, START, StateGraph
from langgraph.graph.message import add_messages
from langgraph.store.base import BaseStore
from langgraph.store.memory import InMemoryStore
from pydantic import BaseModel, Field

MODEL = os.environ.get("MODEL", "deepseek-flash")
CUSTOMER_ID = "C082"


class Profile(BaseModel):
    invoice_title: str | None = Field(
        default=None, description="顾客明确说的发票抬头；没说不填"
    )


class State(TypedDict):
    messages: Annotated[list, add_messages]
    saved: bool


# function_calling 会强制 tool_choice；思考模式不支持，所以关掉思考。
extractor = ChatOpenAI(
    model=MODEL, extra_body={"thinking": {"type": "disabled"}}
).with_structured_output(Profile, method="function_calling")
chat = ChatOpenAI(model=MODEL, extra_body={"thinking": {"type": "disabled"}})

store = InMemoryStore()


def route(state: State) -> str:
    """只有顾客明确说“以后……”时才写档案；问问题不写。"""
    text = state["messages"][-1].content
    return "save_profile" if "以后" in text else "respond"


def save_profile(state: State, config, store: BaseStore) -> dict:
    """单独的写记忆节点：先读旧值，再合并，不覆盖别的字段。"""
    customer_id = config["configurable"]["user_id"]
    thread_id = config["configurable"]["thread_id"]
    said = state["messages"][-1].content
    profile = extractor.invoke(
        [SystemMessage(content="只抽取顾客明确说出的档案字段；没有提到就留空。"),
         HumanMessage(content=said)]
    ).model_dump(exclude_none=True)

    namespace = (customer_id, "profile")
    old = store.get(namespace, "invoice")
    value = dict(old.value) if old else {}
    value.update(profile)
    value["updated"] = "2026-11-17"
    value["source_thread"] = thread_id
    store.put(namespace, "invoice", value)
    return {"saved": True}


def respond(state: State, config, store: BaseStore) -> dict:
    """读记忆只读本顾客的完整 namespace，不把整段历史塞进提示。"""
    customer_id = config["configurable"]["user_id"]
    item = store.get((customer_id, "profile"), "invoice")
    known = json.dumps(item.value, ensure_ascii=False, sort_keys=True) if item else "（空）"
    system = (
        "你是青石书店的客服。下面是这位顾客的档案；顾客问发票抬头时直接复述 invoice_title，"
        "不要编造档案里没有的信息。\n"
        f"已知顾客信息：{known}"
    )
    reply = chat.invoke([SystemMessage(content=system), *state["messages"]])
    return {"messages": [reply]}


builder = StateGraph(State)
builder.add_node("save_profile", save_profile)
builder.add_node("respond", respond)
builder.add_conditional_edges(START, route, ["save_profile", "respond"])
builder.add_edge("save_profile", "respond")
builder.add_edge("respond", END)
graph = builder.compile(checkpointer=InMemorySaver(), store=store)

config1 = {"configurable": {"thread_id": "thread-1", "user_id": CUSTOMER_ID}}
first = graph.invoke({"messages": [HumanMessage(content="以后发票抬头写天津远山文化")]}, config1)
print("thread 1 回答:", first["messages"][-1].content.strip())

item = store.get((CUSTOMER_ID, "profile"), "invoice")
print("store 里的 profile:", json.dumps(item.value, ensure_ascii=False, sort_keys=True))

config2 = {"configurable": {"thread_id": "thread-2", "user_id": CUSTOMER_ID}}
second = graph.invoke({"messages": [HumanMessage(content="我的发票抬头是什么？")]}, config2)
print("thread 2 回答:", second["messages"][-1].content.strip())
print("thread 2 的消息条数:", len(second["messages"]))
