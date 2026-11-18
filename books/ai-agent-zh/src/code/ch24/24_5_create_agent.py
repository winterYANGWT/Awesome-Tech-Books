# 24.5 create_agent：T12“签收后多久能退”
# mode: run
# expect: 15 天
import os

from langchain.agents import create_agent
from langchain_core.tools import StructuredTool
from langchain_openai import ChatOpenAI

from shop import Store, tickets
from shop.tools import SUPPORT_PROMPT, support_tools

MODEL = os.environ.get("MODEL", "deepseek-flash")
box = support_tools(Store(), include=("get_order", "search_policies"))


def as_langchain(t) -> StructuredTool:
    def call(**kwargs):
        return box.call(t.name, kwargs)
    return StructuredTool(name=t.name, description=t.description,
                          args_schema=t.parameters, func=call)


tools = [as_langchain(t) for t in box.tools.values()]
model = ChatOpenAI(model=MODEL, extra_body={"thinking": {"type": "disabled"}})
agent = create_agent(model=model, tools=tools, system_prompt=SUPPORT_PROMPT)

ticket = next(t for t in tickets() if t["id"] == "T12")["text"]
result = agent.invoke({"messages": [("user", ticket)]})
messages = result["messages"]
calls = [tc for m in messages for tc in getattr(m, "tool_calls", [])]

print(f"消息 {len(messages)} 条，工具调用 {len(calls)} 次")
for tc in calls:
    print(f"  {tc['name']}({tc['args']})")
print("最终回答：" + messages[-1].content)
