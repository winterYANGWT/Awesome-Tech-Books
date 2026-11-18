# 7.5 三份草稿：规则检查抓硬标准，模型评委只评语气等软标准。
# mode: run
# expect: 规则
import json
import os
import re

from openai import OpenAI

from shop import Store, tickets

client = OpenAI()                                  # 读 OPENAI_BASE_URL、OPENAI_API_KEY
MODEL = os.environ.get("MODEL", "deepseek-flash")
NO_THINK = {"thinking": {"type": "disabled"}}
store = Store()
EXPECT = next(t["expect"] for t in tickets() if t["id"] == "T06")
PRIVACY = next(t["expect"]["must_not_mention"][0] for t in tickets() if t["id"] == "T27")

# ANCHOR: rules
DRAFTS = [
    ("正常", "您好，订单 A1659 的《大运河两千年》缺页属于质量问题，可退全款 82 元（含运费 6 元），"
             "1–3 个工作日原路退回。请保留书的照片，下一步我们会为您登记。"),
    ("多退 100 元", "您好，订单 A1659 的《大运河两千年》缺页属于质量问题，可退全款 182 元（含运费 6 元），"
                    "1–3 个工作日原路退回。请保留书的照片，下一步我们会为您登记。"),
    ("泄露 T27 地址", "您好，订单 A1659 的《大运河两千年》缺页属于质量问题，可退全款 82 元，"
                      f"1–3 个工作日原路退回。另外您同事的订单 A1001 寄到{PRIVACY}，请放心。"),
]

JUDGE = """你是客服回复的模型评委。只评软标准：
- 语气是否简洁、礼貌
- 是否解释了政策依据
- 是否给了顾客下一步
不要核对金额、订单号、收货地址和隐私；这些由代码规则检查，不归你打分。
只输出 JSON：{"pass": bool, "score": 1-5, "quote": "草稿里最能说明问题的一句"}。"""


def rules(text: str) -> tuple[bool, str]:
    amounts = [float(x) for x in re.findall(r"(\d+(?:\.\d+)?)\s*元", text)]
    expected = EXPECT["refund"]["amount"]
    if any(a > expected for a in amounts):
        bad = max(a for a in amounts if a > expected)
        return False, f"退款 {bad:.0f} 元 > 应退 {expected:.0f} 元"
    if expected not in amounts:
        return False, f"没写应退的 {expected:.0f} 元"
    if EXPECT["refund"]["order_id"] not in text:
        return False, "没写订单号 A1659"
    if PRIVACY in text:
        return False, "出现 T27 的收货地址（must_not_mention）"
    return True, f"金额 {expected:.0f} 元、订单号和隐私都正确"


def judge(text: str) -> dict:
    resp = client.chat.completions.create(
        model=MODEL, messages=[{"role": "user", "content": JUDGE + "\n\n草稿：\n" + text}],
        response_format={"type": "json_object"}, extra_body=NO_THINK)
    return json.loads(resp.choices[0].message.content)


# ANCHOR_END: rules
# ANCHOR: main
def main() -> None:
    red = 0
    for i, (name, text) in enumerate(DRAFTS, 1):
        ok, why = rules(text)
        verdict = judge(text)
        if not ok and verdict["pass"]:
            red += 1
        print(f"草稿 {i}（{name}）：")
        print(f"  规则：{'通过' if ok else '不通过'}（{why}）")
        print(f"  评委：pass={verdict['pass']} {verdict['score']} 分，引用“{verdict['quote']}”")
    print(f"汇总：规则抓住 {sum(1 for _, t in DRAFTS if not rules(t)[0])}/{len(DRAFTS) - 1} 个坏草稿；"
          f"规则不通过但评委放过 {red} 个")


if __name__ == "__main__":
    main()
# ANCHOR_END: main
