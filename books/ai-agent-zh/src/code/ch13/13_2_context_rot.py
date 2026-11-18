# mode: run
# timeout: 600
# expect: 长上下文
"""同一个退货问题，条件 A 只给现行两条条款，条件 B 再塞约 2.4 万 token 的旧版政策和旧工单。"""
import json
import os
import re

from openai import OpenAI

from shop import Store

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")
store = Store()

KEEP = {"退货和退款 / 退货期限", "退货和退款 / 质量问题"}
CURRENT = "\n\n".join(f"## {s['title']}\n{s['text']}" for s in store.policy_sections()
                      if s["doc"] == "returns" and s["title"] in KEEP)
QUESTION = "签收后多少天内可以退货？"

OLD_SECTIONS = [
    ("退货期限", "签收后 7 天内可以申请退货，以物流记录的签收日期为准。超过 7 天不再受理，"
                 "顾客坚持要退的，转交门店值班经理，不要直接答应。"),
    ("七天无理由", "本店执行七天无理由退货：从签收次日起算，第 7 天 24 点前提交申请。"
                   "申请要写清订单号、书名和退货原因；没有订单号的不予受理。"),
    ("质量问题", "缺页、错装、印刷模糊、装订脱落、运输破损都算质量问题。质量问题不受 7 天限制，"
                 "但要在签收后 30 天内提出，并提供照片；超过 30 天的按普通退货处理。"),
    ("教辅和考试书", "教辅、考试用书、字帖拆封后不退；没有拆封的按 7 天期限退。"
                     "学校团购超过 20 套的，要联系团购专员，不走客服流程。"),
    ("退款到账", "审核通过后 3–5 个工作日原路退回。银行卡退款可能延到 7 个工作日；"
                 "支付宝和微信一般 1–3 个工作日。退款金额只算书款，运费不退。"),
    ("换货", "同一本书有质量问题可以换货，换货申请也要在签收后 7 天内提出；"
             "超过 7 天的只能退，不能换。换货寄回的运费由顾客先垫付，收到后随退款返还。"),
    ("门店自提", "门店自提的订单以取货日期代替签收日期，7 天期限从取货当天算起。"
                 "取货后超过 7 天不取的订单自动取消，款项 7 个工作日内退回。"),
    ("特殊订单", "礼品卡、电子书、已拆封的音频课程不退。定制书、签名版、限量版不退不换。"
                 "企业采购开过专用发票的，退货前要先退回发票，否则按 7 天期限拒绝。"),
]
OLD_POLICY = "# 青石书店退货办法（2026-06 版，已废止）\n\n> 2025-03-01 施行，2026-07-01 废止。\n\n" + \
             "\n\n".join(f"## {h}\n{t}" for h, t in OLD_SECTIONS)
REASONS = ["缺页", "装订脱落", "买重复了", "不想要了", "运输破损", "买错版本", "印刷模糊", "教辅拆封"]
VERDICTS = ["签收 9 天，超过 7 天，已拒绝", "签收 5 天，在 7 天内，已受理", "签收 12 天，超过 7 天，转门店经理",
            "签收 2 天，在 7 天内，已退款", "签收 8 天，超过 7 天，已拒绝"]
old_orders = store.query("SELECT id, customer_id, created, status, total FROM orders "
                         "WHERE created < '2026-07-01' ORDER BY created, id LIMIT 170")
tickets = []
for i, o in enumerate(old_orders):
    tickets.append(
        f"工单 OT-{i + 1:03d}（{o['created']}）：顾客 {o['customer_id']} 就订单 {o['id']}"
        f"（{o['total']:.2f} 元，{o['status']}）提出退货，理由是{REASONS[i % len(REASONS)]}。"
        f"客服核对物流签收日期后按 7 天期限判断：{VERDICTS[i % len(VERDICTS)]}。"
        f"处理记录：已向顾客说明本店退货期限为签收后 7 天，超过 7 天不能按普通退货办理，"
        f"也不适用七天无理由；顾客如有异议，记录诉求后转交值班经理，不当场承诺退款。"
        f"回访结果：顾客{'接受' if i % 3 else '不接受'}按 7 天期限的处理。")
OLD_ARCHIVE = OLD_POLICY + "\n\n## 旧工单记录（2026 年上半年）\n\n" + "\n\n".join(tickets)

PROMPT = "你是青石书店的客服。只根据下面给出的资料回答，答案尽量短。"
A = [{"role": "system", "content": PROMPT + "\n\n# 现行退货条款\n\n" + CURRENT},
     {"role": "user", "content": QUESTION}]
B = [{"role": "system", "content": PROMPT + "\n\n# 现行退货条款\n\n" + CURRENT +
      "\n\n# 存档资料（旧版政策和旧工单）\n\n" + OLD_ARCHIVE},
     {"role": "user", "content": QUESTION}]


def ask(messages):
    resp = client.chat.completions.create(model=MODEL, messages=messages,
                                          extra_body={"thinking": {"type": "disabled"}}, max_tokens=120)
    return resp.choices[0].message.content.strip(), resp.usage.prompt_tokens


def right(answer):
    if "15" not in answer and "十五" not in answer:
        return False
    if not re.search(r"[7七]\s*天", answer):
        return True
    return bool(re.search(r"(前|以前|之前|旧版|历史|2026-07-01).{0,24}[7七]\s*天", answer) or
                re.search(r"[7七]\s*天.{0,24}(前|以前|之前|旧版|历史)", answer))


for name, messages in (("A", A), ("B", B)):
    ok, tokens, old = 0, [], 0
    print(f"条件 {name}（{'短上下文：只给现行条款' if name == 'A' else '长上下文：现行条款 + 旧版政策 + 旧工单'}）")
    for i in range(10):
        answer, used = ask(messages)
        tokens.append(used)
        ok += right(answer)
        old += bool(re.search(r"[7七]\s*天", answer))
        print(f"  {name}{i + 1:02d} {' '.join(answer.split())[:24]:<26}{'√' if right(answer) else '×'}")
    print(f"条件 {name} 答对 {ok}/10，提到 7 天 {old}/10，平均 prompt {round(sum(tokens) / len(tokens)):,}")
