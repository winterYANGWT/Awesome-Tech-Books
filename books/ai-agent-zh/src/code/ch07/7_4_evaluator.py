# 7.4 生成者写草稿，评估者挑问题，生成者按 issues 改，最多 2 轮。
# mode: run
# expect: 主管
import json
import os

from openai import OpenAI

client = OpenAI()                                  # 读 OPENAI_BASE_URL、OPENAI_API_KEY
MODEL = os.environ.get("MODEL", "deepseek-flash")
NO_THINK = {"thinking": {"type": "disabled"}}

FACTS = """订单 A1680：实付 680.00 元，已退 0.00 元，状态已签收，2026-10-29 签收。
商品：《精装典藏版：世界文学名著十二卷》1 套，顾客说其中三卷装订脱落。
政策：装订脱落属于质量问题，60 天内可退全款；单笔退款超过 500 元需要主管批准；
退款原路返回，1–3 个工作日到账。顾客要求：全额退款，今天到账。"""

# ANCHOR: loop
EVALUATOR = """你是退款回复的评估者。你不改写草稿，只输出 JSON：
{"pass": bool, "score": 1-5, "issues": [{"type": str, "quote": str, "fix": str}]}
检查项与 type：
- over_limit：金额是否超过 500 元；超过就必须写“需主管批准”
- unauthorized_promise：是否承诺“今天到账/今天退款/立即退款”等政策不允许的事
- missing_supervisor：是否明确说要提交主管审批
- tone：语气是否简洁、是否给顾客下一步
pass 为 true 的条件：金额 680 元正确；明确说明超过 500 元需主管批准并提交主管；
没有承诺今天到账或直接退款。否则 pass=false，每条 issue 的 quote 必须抄草稿原句。"""


def generate(bad_first: bool) -> str:
    if bad_first:
        prompt = FACTS + """

写一版给顾客的回复草稿，不超过 120 字。为了演示评估者，这一版必须直接答应
“今天退 680 元”，不要提主管批准。"""
    else:
        prompt = FACTS + """

上一版草稿被评估者退回了。按下面的 issues 逐条修改，重写一版不超过 120 字的回复。
issues：""" + json.dumps(ISSUES, ensure_ascii=False)
    resp = client.chat.completions.create(
        model=MODEL, messages=[{"role": "user", "content": prompt}], max_tokens=300,
        extra_body=NO_THINK)
    return resp.choices[0].message.content.strip()


def evaluate(draft: str) -> dict:
    prompt = EVALUATOR + "\n\n草稿：\n" + draft
    resp = client.chat.completions.create(
        model=MODEL, messages=[{"role": "user", "content": prompt}],
        response_format={"type": "json_object"}, extra_body=NO_THINK)
    return json.loads(resp.choices[0].message.content)


# ANCHOR_END: loop
def show_round(n: int, verdict: dict) -> None:
    print(f"第 {n} 轮评估：pass={verdict['pass']} score={verdict['score']}")
    for issue in verdict.get("issues", []):
        print(f"  [{issue.get('type')}] “{issue.get('quote')}” → {issue.get('fix')}")


draft = generate(bad_first=True)
print("第 1 版草稿：" + draft)
verdict = evaluate(draft)
show_round(1, verdict)

if not verdict["pass"]:
    ISSUES = verdict.get("issues", [])
    draft = generate(bad_first=False)
    print("第 2 版草稿：" + draft)
    verdict = evaluate(draft)
    show_round(2, verdict)

print(f"最终：pass={verdict['pass']}")
if not verdict["pass"]:
    print("两轮仍未通过：停止修改，转人工")
