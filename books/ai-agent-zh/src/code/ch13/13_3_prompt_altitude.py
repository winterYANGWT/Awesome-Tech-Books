# mode: run
# timeout: 480
# expect: if-else
"""同一个工单集，三种“高度”的系统提示：if-else、泛泛版、5 条规则版。"""
import os

from openai import OpenAI

from harness import run
from shop import Store, tickets
from shop.tools import SUPPORT_PROMPT, support_tools

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")
IDS = ["T06", "T09", "T12", "T15", "T19", "T24"]
ALL = {t["id"]: t for t in tickets()}

VAGUE = "你是青石书店的客服，帮顾客解决问题。回答简洁。"

IFELSE = """你是青石书店的客服。严格按下面的规则处理，不要自行判断，也不要查政策文档：
1. 顾客说缺页、装订脱落、印刷模糊 → 质量问题，退全款。
2. 顾客说买重复、买错、不想要 → 只退书款，运费不退。
3. 退货期限：签收后 7 天内，超过 7 天一律拒绝。
4. 教辅、考试书拆封 → 不退。
5. 订单金额超过 500 元 → 直接退款，不用问主管。
6. 广州、广西、海南的快递 → 顺路快递。
7. 积分 100 分抵 0.5 元。
8. 地址修改 → 已发货也能改。
9. 退款到账 → 7 个工作日。
10. 顾客问物流 → 直接说“明天到”。
11. 顾客问发票 → 说“不支持”。
12. 顾客问会员 → 说“没有会员”。
13. 顾客问优惠 → 说“没有优惠”。
14. 顾客问换货 → 说“只能退”。
15. 顾客问运费 → 说“一律 6 元”。
16. 顾客问签收 → 说“三天内签收”。
17. 顾客问破损 → 说“拍照发邮箱”。
18. 顾客问少发 → 说“补发”。
19. 顾客问多发 → 说“留着”。
20. 顾客问取消 → 说“不能取消”。
21. 顾客问改书 → 说“不能改”。
22. 顾客问加急 → 说“加 20 元”。
23. 顾客问自提 → 说“不支持”。
24. 顾客问发票抬头 → 说“个人”。
25. 顾客问税号 → 说“不需要”。
26. 顾客问退款进度 → 说“等着”。
27. 顾客问投诉 → 说“记下了”。
28. 顾客问主管 → 说“主管不在”。
29. 顾客问电话 → 说“不方便”。
30. 顾客问地址 → 说“不能改”。
31. 顾客问书名 → 说“自己看”。
32. 顾客问作者 → 说“不知道”。
33. 顾客问出版社 → 说“不知道”。
34. 顾客问价格 → 说“以页面为准”。
35. 顾客问库存 → 说“有货”。
36. 顾客问发货 → 说“今天发”。
37. 顾客问快递单号 → 说“稍后发”。
38. 顾客问签收人 → 说“本人”。
39. 顾客问退货寄回 → 说“寄到付”。
40. 任何拿不准的事 → 回答“请稍等”。"""

PROMPTS = [("if-else", IFELSE), ("泛泛版", VAGUE), ("4 条规则", SUPPORT_PROMPT)]


def judge(t, answer, actions):
    exp = t["expect"]
    refunds = [a for a in actions if a["kind"] == "refund"]
    adds = [a for a in actions if a["kind"] == "update_address"]
    esc = [a for a in actions if a["kind"] == "escalate"]
    if exp["refund"] is None:
        if refunds:
            return False
    elif not any(a["order_id"] == exp["refund"]["order_id"] and abs(a["detail"]["amount"] - exp["refund"]["amount"]) < 0.01
                 for a in refunds):
        return False
    if exp["address_changed"] is None:
        if adds:
            return False
    elif not any(a["order_id"] == exp["address_changed"]["order_id"] and exp["address_changed"]["contains"] in a["detail"]["new"]
                 for a in adds):
        return False
    if exp.get("escalate") and not esc:
        return False
    if any(w not in answer for w in exp["mention"]):
        return False
    if any(w in answer for w in exp.get("must_not_mention", [])):
        return False
    return True


print("提示版本      正确   每轮 prompt   每工单累计   工具调用   逐题")
for name, prompt in PROMPTS:
    ok, tokens, calls, turns, marks = 0, 0, 0, 0, []
    for tid in IDS:
        store = Store()
        res = run([{"role": "system", "content": prompt}, {"role": "user", "content": ALL[tid]["text"]}],
                  support_tools(store), model=MODEL, max_turns=6, verbose=False,
                  extra_body={"thinking": {"type": "disabled"}})
        good = judge(ALL[tid], res.answer or "", store.actions())
        ok += good
        tokens += res.usage.get("prompt_tokens", 0)
        calls += len(res.steps)
        turns += res.turns
        marks.append(f"{tid}{'✓' if good else '✗'}")
    print(f"{name:<12}{ok}/6    {round(tokens / turns):>8,}   {round(tokens / 6):>9,}   {calls:>7}   {' '.join(marks)}")
