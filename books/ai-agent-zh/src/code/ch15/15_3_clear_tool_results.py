# mode: run
# expect: cleared=
"""同一段 8 轮对话：先把 6 条旧工具结果的正文换成 stub，再回答最后的问题。

只改 content，不删消息；tool_call_id 和 assistant 的 tool_calls 都还在。
"""
import json
import os

from openai import OpenAI

from lib import check_pairs, clear_tool_results, count_request
from shop import Store
from shop.tools import SUPPORT_PROMPT, support_tools

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")
NO_THINK = {"thinking": {"type": "disabled"}}
store = Store()
box = support_tools(store)
TOOLS = box.schemas()


def call(cid: str, name: str, args: dict) -> dict:
    return {"role": "assistant", "content": "", "tool_calls": [
        {"id": cid, "type": "function",
         "function": {"name": name, "arguments": json.dumps(args, ensure_ascii=False)}}]}


# 一条很长的旧结果：顾客要核对全部历史订单，工具把 90 张都导了出来。
old = store.query("SELECT id, created, status, total FROM orders ORDER BY id LIMIT 90")
LOG = json.dumps([{"order_id": o["id"], "date": o["created"], "status": o["status"],
                   "total": o["total"],
                   "note": f"旧工单：顾客询问退货，已按当时期限答复；处理人客服{i % 7 + 1}，"
                           f"回访{'完成' if i % 2 else '未完成'}。"}
                  for i, o in enumerate(old)], ensure_ascii=False)

messages = [
    {"role": "system", "content": SUPPORT_PROMPT},
    {"role": "user", "content": "你好，我这几年的订单有点多，先把我全部订单导出来，我要一张一张核对。"
                                "我是 C082，手机尾号 8842。"},
    call("c_log", "order_log", {}),
    {"role": "tool", "tool_call_id": "c_log", "content": LOG},
    {"role": "assistant", "content": f"已经导出旧订单记录，共 {len(old)} 张，最早从 {old[0]['id']} 开始。"},
    {"role": "user", "content": "收到了。另外 11 月 7 号那单 A1659《大运河两千年》缺了二十多页，"
                                "第 180 到 205 页是白的，后面还重复。我要退款，钱退回我原来那张卡，"
                                "不要退到账户余额。我白天在车间上班，接不了电话，有事发短信；"
                                "这周五之前要处理完。"},
    call("c_a1659", "get_order", {"order_id": "A1659"}),
    {"role": "tool", "tool_call_id": "c_a1659", "content": box.call("get_order", {"order_id": "A1659"})},
    call("c_pol", "search_policies", {"query": "质量问题 退货"}),
    {"role": "tool", "tool_call_id": "c_pol", "content": box.call("search_policies", {"query": "质量问题 退货"})},
    {"role": "assistant", "content": "A1659 实付 82.00 元，缺页属于质量问题，可以退全款，书不用寄回。"},
    {"role": "user", "content": "这算质量问题吗？政策上具体怎么写的？以后再遇到这种情况我也好判断。"},
    call("c_pol2", "search_policies", {"query": "退货期限 质量问题 60 天"}),
    {"role": "tool", "tool_call_id": "c_pol2",
     "content": box.call("search_policies", {"query": "退货期限 质量问题 60 天"})},
    {"role": "assistant", "content": "政策写明缺页、错装、印刷模糊、装订脱落、运输破损都算质量问题；"
                                    "质量问题退全款，不受 15 天限制，签收后 60 天内都能申请。"},
    {"role": "user", "content": "那退全款到底退多少？我实付里是不是含运费？别只退书款。"},
    call("c_a1659b", "get_order", {"order_id": "A1659"}),
    {"role": "tool", "tool_call_id": "c_a1659b", "content": box.call("get_order", {"order_id": "A1659"})},
    {"role": "assistant", "content": "书款 76.00 元，运费 6.00 元，实付合计 82.00 元；"
                                    "质量问题退全款，所以退 82.00 元，不是只退 76.00。"},
    {"role": "user", "content": "行，那就按缺页退全款吧。我那张卡还是原来的，别改成余额。"},
    call("c_refund", "issue_refund", {"order_id": "A1659", "amount": 82, "reason": "缺页"}),
    {"role": "tool", "tool_call_id": "c_refund",
     "content": box.call("issue_refund", {"order_id": "A1659", "amount": 82, "reason": "缺页"})},
    {"role": "assistant", "content": "退款已经提交成功：A1659 退 82.00 元，订单状态已退款。"},
    {"role": "user", "content": "退款多久能到账？周五之前能到吗？我那边财务要登记。"},
    call("c_pol3", "search_policies", {"query": "退款 到账 时间"}),
    {"role": "tool", "tool_call_id": "c_pol3", "content": box.call("search_policies", {"query": "退款 到账 时间"})},
    {"role": "assistant", "content": "政策写的是退款原路返回，1–3 个工作日到账；今天周二，周五之前能到。"},
    {"role": "user", "content": "我上次那单 A1469 退的 48 元到账了吗？别和这次弄混。"},
    call("c_a1469", "get_order", {"order_id": "A1469"}),
    {"role": "tool", "tool_call_id": "c_a1469", "content": box.call("get_order", {"order_id": "A1469"})},
    {"role": "assistant", "content": "A1469 实付 86.00 元，已退 48.00 元，状态是部分退款；"
                                    "这是另一笔，不会和 A1659 的 82.00 元混在一起。"},
    {"role": "user", "content": "好，那你最后帮我确认一下：这次 A1659 退了多少、退到哪、什么时候到账？"},
]

check_pairs(messages)
before = count_request(messages, TOOLS)
cleared, n = clear_tool_results(messages, keep_last=2)
after = count_request(cleared, TOOLS)
print(f"cleared={n} before≈{before:,} after≈{after:,} 消息={len(messages)}→{len(cleared)}")
print("留下的 stub：")
for m in cleared:
    if m["role"] == "tool" and m["content"].startswith("[已清理"):
        print("  " + m["content"])

resp = client.chat.completions.create(model=MODEL, messages=cleared, max_tokens=600, extra_body=NO_THINK)
answer = resp.choices[0].message.content or ""
print(f"第 8 轮 prompt={resp.usage.prompt_tokens}")
print("答案：" + " ".join(answer.split()))
dest = any(w in answer for w in ("原卡", "原银行卡", "原路", "原来那张卡"))
print(f"答案检查：82.00={'82.00' in answer} 去向={'对' if dest else '错'}")
