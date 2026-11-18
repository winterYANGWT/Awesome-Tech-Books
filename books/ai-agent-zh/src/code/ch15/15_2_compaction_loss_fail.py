# mode: fail
# expect: LostConstraint
"""压缩反例：同一段 12 轮对话、同一个 PLAN、同样保留最近 4 条，只把摘要提示
换成“只留订单号、金额、原因”，看第 7 轮丢掉的约束会不会传到最终答案。

第 12 轮才检查，所以前面几轮看起来一切正常；最终答案只能按最近的话题猜。
"""
import json
import os

from openai import OpenAI

from lib import LOSSY_SYSTEM, compact, count_request, summarize
from shop import Store
from shop.tools import SUPPORT_PROMPT, support_tools

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")
NO_THINK = {"thinking": {"type": "disabled"}}
store = Store()
box = support_tools(store)
TOOLS = box.schemas()


class LostConstraint(Exception):
    pass


USER = [
    "你好，我是 C082。11 月 7 号在你们店买了《大运河两千年》，订单 A1659，11 月 11 号签收。"
    "翻开发现中间缺了二十多页，第 180 到 205 页是白的，后面还重复了。我要退款，钱退回我原来那张卡，"
    "不要退到账户余额。我白天在车间上班，接不了电话，有事发短信。这周五之前要处理完，"
    "麻烦你直接告诉我怎么办，别让我再找第二个人。",
    "这算质量问题吗？政策上怎么写的？要不要把书寄回去？",
    "那退全款是退多少？我实付多少？书价和运费分别多少？",
    "行，那就按缺页退全款吧。我那张卡还是原来的，别改成余额。",
    "退款多久能到账？周五之前能到吗？",
    "我上次那单 A1469 退的 48 元到账了吗？别和这次弄混。",
    "我还有其他几单，A1580、A1152，能一起看看吗？",
    "A1580 那本我没拆，能退吗？",
    "这本书定价多少？我记得打完折不是这个数。",
    "我账户余额里还有几十块，下次买书能直接抵吗？",
    "那这次退款到底退到哪？我卡号没变，也不想要余额。",
    "帮我总结一下：这笔退款退了多少、退到哪、什么时候到账？",
]
PLAN = [
    [("get_order", {"order_id": "A1659"}), ("search_policies", {"query": "质量问题 退货"})],
    [("search_policies", {"query": "退款 到账 时间"})],
    [("get_order", {"order_id": "A1659"})],
    [("issue_refund", {"order_id": "A1659", "amount": 82, "reason": "缺页"})],
    [],
    [("get_order", {"order_id": "A1469"})],
    [("get_customer_context", {"customer_id": "C082"})],
    [("get_order", {"order_id": "A1580"})],
    [("search_books", {"query": "大运河两千年"})],
    [],
    [],
    [],
]


def add_tools(messages: list, calls: list, turn: int) -> None:
    ids = [f"call_fixed_{turn}_{i}" for i in range(len(calls))]
    messages.append({"role": "assistant", "content": "", "tool_calls": [
        {"id": cid, "type": "function",
         "function": {"name": name, "arguments": json.dumps(args, ensure_ascii=False)}}
        for cid, (name, args) in zip(ids, calls)]})
    for cid, (name, args) in zip(ids, calls):
        messages.append({"role": "tool", "tool_call_id": cid, "content": box.call(name, args)})


messages = [{"role": "system", "content": SUPPORT_PROMPT}]
summary = ""
for turn, text in enumerate(USER, 1):
    if turn in (7, 11):
        before = count_request(messages, TOOLS)
        summary, su = summarize(client, messages, MODEL, system=LOSSY_SYSTEM)
        messages = compact(messages, summary, keep_last=4)
        after = count_request(messages, TOOLS)
        print(f"第 {turn} 轮压缩：before≈{before:,} after≈{after:,} "
              f"摘要调用={su.get('prompt_tokens', 0)}+{su.get('completion_tokens', 0)}")
        print("摘要：" + " ".join(summary.split()))
    messages.append({"role": "user", "content": text})
    kwargs = {"tools": TOOLS} if turn < 12 else {}
    resp = client.chat.completions.create(model=MODEL, messages=messages, max_tokens=700,
                                          extra_body=NO_THINK, **kwargs)
    answer = resp.choices[0].message.content or ""
    print(f"第 {turn} 轮 prompt={resp.usage.prompt_tokens}")
    messages.append({"role": "assistant", "content": answer})
    if turn == 12:
        print("答案：" + " ".join(answer.split()))
    elif PLAN[turn - 1]:
        add_tools(messages, PLAN[turn - 1], turn)

dest = any(w in answer for w in ("原卡", "原银行卡", "原来的银行卡"))
print(f"检查：摘要原卡={'原卡' in summary} 答案原卡={dest} 短信={'短信' in answer}")
missing = []
if not dest:
    missing.append("退回原来的卡")
if "短信" not in answer:
    missing.append("只发短信")
if missing:
    raise LostConstraint("顾客第 1 轮要求" + "、".join(missing) + "，摘要只留了订单号、金额和原因；"
                         f"最终答案没有这条约束：{' '.join(answer.split())[:60]}")
