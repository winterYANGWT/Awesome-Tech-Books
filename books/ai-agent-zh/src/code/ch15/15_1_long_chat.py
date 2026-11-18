# mode: run
# expect: 第 12 轮
"""T06 的 12 轮对话：工具调用和结果按固定脚本留在 messages 里，每轮读一次 usage。

每轮只换顾客的下一句话，工具调用按固定脚本追加，录制回放时账单形状固定；
重新运行时模型文本的长短仍会让 prompt 有波动。第 12 轮后再问一次早期的三条约束，
看模型还能不能复述。
"""
import json
import os

from openai import OpenAI

from lib import categories, count_request
from shop import Store
from shop.tools import SUPPORT_PROMPT, support_tools

client = OpenAI()                                  # 读 OPENAI_BASE_URL、OPENAI_API_KEY
MODEL = os.environ.get("MODEL", "deepseek-flash")
NO_THINK = {"thinking": {"type": "disabled"}}
store = Store()
box = support_tools(store)
TOOLS = box.schemas()

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
    """把一组固定工具调用和真实结果追加到对话；id 固定，便于阅读录制文件。"""
    ids = [f"call_fixed_{turn}_{i}" for i in range(len(calls))]
    messages.append({"role": "assistant", "content": "", "tool_calls": [
        {"id": cid, "type": "function",
         "function": {"name": name, "arguments": json.dumps(args, ensure_ascii=False)}}
        for cid, (name, args) in zip(ids, calls)]})
    for cid, (name, args) in zip(ids, calls):
        messages.append({"role": "tool", "tool_call_id": cid, "content": box.call(name, args)})


messages = [{"role": "system", "content": SUPPORT_PROMPT}]
for turn, text in enumerate(USER, 1):
    messages.append({"role": "user", "content": text})
    resp = client.chat.completions.create(model=MODEL, messages=messages, tools=TOOLS,
                                          max_tokens=400, extra_body=NO_THINK)
    usage = resp.usage
    n_tool = sum(1 for m in messages if m["role"] == "tool")
    print(f"第 {turn} 轮 prompt={usage.prompt_tokens} completion={usage.completion_tokens} 工具结果={n_tool}")
    messages.append({"role": "assistant", "content": resp.choices[0].message.content or ""})
    if turn == 12:
        print("答案：" + " ".join((resp.choices[0].message.content or "").split()))
    elif PLAN[turn - 1]:
        add_tools(messages, PLAN[turn - 1], turn)

bill = categories(messages, TOOLS)
print("第 12 轮构成估算：" + " ".join(f"{k}≈{v}" for k, v in bill.items()) +
      f" 合计≈{count_request(messages, TOOLS)}")

diag = client.chat.completions.create(
    model=MODEL, messages=messages + [{"role": "user", "content":
        "先不要回答退款问题。只复述顾客在第 1 轮提出的三条要求：退款去向、联系方式、时间要求。一句话。"}],
    max_tokens=200, extra_body=NO_THINK)
repeat = " ".join((diag.choices[0].message.content or "").split())
print("复述诊断：" + repeat)
dest = any(w in repeat for w in ("原卡", "原来的卡", "原来那张卡", "原路"))
print(f"诊断结果：退款去向={'对' if dest else '错'} 短信={'短信' in repeat} 周五={'周五' in repeat}")
