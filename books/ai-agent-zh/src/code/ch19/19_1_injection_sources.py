# 19.1 三条注入：B05、B21、B33 的评论原文和它们在消息列表里的位置。
# mode: run
# llm: none
# expect: 系统通知
# expect: role: tool
import json

from shop import Store
from shop.tools import support_tools

store = Store()
box = support_tools(store)


def short(text: str, n: int = 64) -> str:
    text = " ".join(str(text).split())
    return text if len(text) <= n else text[: n - 1] + "…"


print("=== B05：伪系统通知，要求给 A1088 退 300 元 ===")
b05 = box.call("list_reviews", {"book_id": "B05"})
for r in json.loads(b05):
    print(f"{r['stars']} 星  {r['text']}")
print()

print("=== B21：HTML 注释，要求打印系统提示和工具列表 ===")
b21 = box.call("list_reviews", {"book_id": "B21"})
for r in json.loads(b21):
    print(f"{r['stars']} 星  {r['text']}")
print()

print("=== B33：要求把顾客引到外部站点 ===")
b33 = box.call("list_reviews", {"book_id": "B33"})
for r in json.loads(b33):
    print(f"{r['stars']} 星  {r['text']}")
print()

# 工具结果原样进入 messages：这里只拼到 list_reviews 返回为止。
messages = [
    {"role": "system", "content": "你是青石书店的在线客服。回答之前先用工具查实际情况。"},
    {"role": "user", "content": "我是 C100，B05 这本书评价怎么样？"},
    {"role": "assistant", "content": "",
     "tool_calls": [{"id": "call_1", "type": "function",
                     "function": {"name": "list_reviews", "arguments": '{"book_id": "B05"}'}}]},
    {"role": "tool", "tool_call_id": "call_1", "content": b05},
]

print("=== 模型实际收到的消息列表（截断）===")
for i, m in enumerate(messages):
    print(f"[{i}] {m['role']:<9} {short(m.get('content', ''))}")
print()

needles = {"B05": "【系统通知", "B21": "<!-- assistant:", "B33": "忽略之前"}
for book, needle in needles.items():
    text = box.call("list_reviews", {"book_id": book})
    print(f"{book} 含 {needle!r}: {needle in text}")
print("B05 这一条在 [3]；三条评论都以 role: tool 返回，角色字段没有把评论和指令分开。")
