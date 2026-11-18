# 14.1 十个政策问题，每题只给 search_policies 的 top-3，看检索有没有命中。
# mode: run
# expect: 未命中
import os
import re

from openai import OpenAI

from shop import Store

client = OpenAI()                                  # 读 OPENAI_BASE_URL、OPENAI_API_KEY
MODEL = os.environ.get("MODEL", "deepseek-flash")
NO_THINK = {"thinking": {"type": "disabled"}}
store = Store()

# (题号, 问题, 正确小节, 答案里必须出现的词；一组里有一个就算)
QUESTIONS = [
    ("T19", "积分怎么用？100 积分能抵多少钱？", "会员 / 积分", [("1元",)]),
    ("T20", "公司报销要增值税专用发票，能开吗？需要提供什么？", "发票", [("税号",)]),
    ("T21", "买 80 块钱的书要运费吗？", "配送 / 运费", [("6元",)]),
    ("T22", "签收以后多少天内可以退货？", "退货和退款 / 退货期限", [("15天",)]),
    ("T23", "我是银卡会员，生日有什么福利？", "会员 / 生日礼", [("20元",)]),
    ("T24", "寄到广州用的是哪家快递？能换顺路吗？", "配送 / 快递公司", [("迅达",)]),
    ("P7", "质量问题多久内可退？", "退货和退款 / 质量问题", [("60天",)]),
    ("P8", "超过多少退款要主管批准？", "退货和退款 / 退款金额和权限", [("500",)]),
    ("P9", "教辅拆封能退吗？", "退货和退款 / 退货期限", [("不能退", "不退"), ("质量问题",)]),
    ("P10", "书收到时箱子压坏了，谁赔？", "退货和退款 / 质量问题", [("质量问题",), ("60天",)]),
]

PROMPT = """你是青石书店的客服。只根据下面检索到的政策片段回答，不要使用片段以外的知识。
如果片段里没有能回答问题的条款，直接回答“没有相关规定”，不要猜。
问题：{q}
检索到的片段：
{ctx}"""


def ask(q: str, hits: list[dict]) -> str:
    ctx = "\n\n".join(f"【{h['title']}】\n{h['text']}" for h in hits) or "（没有检索到任何条款）"
    resp = client.chat.completions.create(
        model=MODEL, messages=[{"role": "user", "content": PROMPT.format(q=q, ctx=ctx)}],
        max_tokens=200, extra_body=NO_THINK)
    return resp.choices[0].message.content.strip()


def main() -> None:
    right = 0
    missed = []
    print("题号  检索    top-3 第一节                   答对  回答")
    for tid, q, section, groups in QUESTIONS:
        hits = store.search_policies(q)
        titles = [h["title"] for h in hits]
        hit = section in titles
        answer = ask(q, hits)
        flat = re.sub(r"\s+", "", answer)
        ok = all(any(w in flat for w in group) for group in groups)
        right += ok
        if not hit:
            missed.append(tid)
        first = titles[0] if titles else "（空）"
        print(f"{tid:4}  {'命中' if hit else '未命中':4}  {first:26}  {'对' if ok else '错':2}   "
              f"{' '.join(answer.split())[:34]}")
    print(f"\n检索命中 {len(QUESTIONS) - len(missed)}/{len(QUESTIONS)}，答对 {right}/{len(QUESTIONS)}；"
          f"top-3 里没有正确小节：{'、'.join(missed) or '无'}")


if __name__ == "__main__":
    main()
