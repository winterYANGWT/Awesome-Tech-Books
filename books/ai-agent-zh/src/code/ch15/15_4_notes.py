# mode: run
# expect: 680
"""T13（A1680，680 元，装订脱落）跑 12 轮：第 2、6 轮写笔记，第 9 轮重置上下文。

笔记是 agent 自己的状态，不是给顾客的回复；重置后只保留系统提示、笔记和最近 2 条。
"""
import os
from typing import Annotated

from openai import OpenAI

from harness import ToolError, run, tool
import lib
from shop import Store, tickets
from shop.tools import SUPPORT_PROMPT, support_tools

MODEL = os.environ.get("MODEL", "deepseek-flash")
NO_THINK = {"thinking": {"type": "disabled"}}
store = Store()
box = support_tools(store)
T13 = next(t for t in tickets() if t["id"] == "T13")


WRITE_TURNS = {2, 6}
CURRENT = {"turn": 0}


@tool
def read_notes() -> str:
    """读 NOTES.md：目标、已完成、待办、关键事实、来源。上下文重置后先用它恢复状态。"""
    return lib.read_notes()


@tool
def write_notes(text: Annotated[str, "完整笔记，必须含五节：## 目标 / ## 已完成 / ## 待办 / ## 关键事实 / ## 来源"]) -> str:
    """整文件替换 NOTES.md。只写里程碑：拿到订单、确认政策、提交主管、退款完成。"""
    if CURRENT["turn"] not in WRITE_TURNS:
        return f"第 {CURRENT['turn']} 轮不是里程碑，笔记没有改动；等到第 2、6 轮再写。"
    try:
        return lib.write_notes(text)
    except Exception as e:  # NotesError 也交给模型，让它补齐五节
        raise ToolError(str(e)) from None


TICKETS: dict[str, str] = {}


@tool
def escalate_to_supervisor(order_id: Annotated[str, "相关订单号，没有就填空字符串"],
                           note: Annotated[str, "需要主管处理的事项，写清楚顾客诉求和你的判断"]) -> dict:
    """把需要主管处理的事项提交给主管。同一订单只提交一次；重复调用返回已有工单。"""
    if order_id in TICKETS:
        return {"ticket": TICKETS[order_id], "status": "已提交主管（此前已提交，未重复创建）"}
    out = store.escalate(order_id or None, note)
    TICKETS[order_id] = out["ticket"]
    return out


box.add(read_notes)
box.add(write_notes)
box.add(escalate_to_supervisor)

SYSTEM = SUPPORT_PROMPT + """

长任务约定：
- 每到一个里程碑（拿到订单、确认政策、提交主管、退款完成），用 read_notes 看旧笔记，再用 write_notes 整份更新。
- 笔记固定五节：## 目标、## 已完成、## 待办、## 关键事实、## 来源；整份控制在 500 token 以内。
- 笔记是给你自己看的，不写内部猜测和其他顾客信息。"""

USER = [
    T13["text"],
    "这单超过 500 元，是不是要主管批准？先把目前的进展写进 NOTES.md，五节都要有。",
    "那就提交主管吧，写清楚三卷装订脱落、顾客要求全额退款。",
    "提交了吗？主管那边大概什么时候能批？",
    "我这边比较急，能不能今天给个答复？",
    "把现在的状态更新到 NOTES.md，尤其是主管审批这一步。",
    "我换了个手机，刚才的聊天记录可能断了，你还在吗？",
    "在的话先别重复问我订单号，我不想再说一遍。",
    "我们刚才说到哪了？我的订单和退款进度。",
    "退款金额到底是多少？",
    "主管批了吗？",
    "给我一个最终答复，我要截图保存。",
]

messages = [{"role": "system", "content": SYSTEM}]
for turn, text in enumerate(USER, 1):
    CURRENT["turn"] = turn
    if turn == 9:
        before_n = len(messages)
        notes = lib.read_notes()
        messages = [messages[0], {"role": "system", "content": "## 当前 NOTES.md\n" + notes}] + messages[-2:]
        print(f"第 9 轮重置：消息 {before_n}→{len(messages)} 条，笔记≈{lib.text_tokens(notes)} token")
    messages.append({"role": "user", "content": text})
    res = run(messages, box, model=MODEL, max_turns=6, verbose=False,
              extra_body=NO_THINK)
    calls = ", ".join(f"{s.name}" for s in res.steps) or "无工具"
    for s in res.steps:
        if s.name == "write_notes":
            print("  write_notes → " + " ".join(s.result.split())[:60])
    answer = " ".join((res.answer or "").split())
    if turn == 12:
        print("最终答复：" + answer)
    else:
        print(f"第 {turn} 轮 {calls}  答：{answer[:70]}")

print(f"NOTES.md：{lib.NOTES.name} ≈{lib.text_tokens(lib.read_notes())} token")
print(lib.read_notes())
print("actions：" + "; ".join(f"{a['kind']}({a['order_id']})" for a in store.actions()))
print(f"最终检查：680={'680' in answer} 主管={'主管' in answer}")
