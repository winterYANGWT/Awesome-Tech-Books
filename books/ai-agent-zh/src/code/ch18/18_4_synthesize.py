# 18.4 把三份 findings 合成 5 段报告，再逐段核对数字和来源。
# mode: run
# expect: 24.6%
import os

from openai import OpenAI

from ch18.lib.common import UncitedClaim, check_citations
from ch18.lib.fixtures import FINDINGS, all_findings

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")
QUESTION = "2026 年 Q3 退货率为什么上升，Q4 为什么回落？"

PROMPT = """你是调研汇总 agent。根据下面的 findings 写 5 段中文报告，回答：Q3 为什么上升、Q4 为什么回落。
硬要求：
1. 只写 5 段，段间空一行；不要标题、列表、代码块。
2. 每段至少一个数字；数字只能来自 findings，不要自己算新数字。
3. 每段末尾标 [数据]、[政策] 或 [评论]，和内容对应。
4. 第 1 段给 Q3、Q4 的退货率；第 2 段写 6 月那批书；第 3 段写快递破损率；
   第 4 段写次因和 Q4 的边界；第 5 段写 gap。每段带一个反例或边界。
5. 没有数据的句子删掉；不要用“可能”“主要是”撑篇幅。"""


def main() -> None:
    evidence = "\n".join(f"- {f['claim']} 证据：{f['evidence']} 来源：{f['source']}"
                         for f in all_findings())
    report = ""
    for attempt in (1, 2):
        resp = client.chat.completions.create(
            model=MODEL,
            messages=[{"role": "system", "content": PROMPT},
                      {"role": "user", "content": f"问题：{QUESTION}\n\nfindings：\n{evidence}"}],
            max_tokens=1400, extra_body={"thinking": {"type": "disabled"}})
        report = (resp.choices[0].message.content or "").strip()
        try:
            n = check_citations(report, all_findings())
            break
        except UncitedClaim as e:
            print(f"第 {attempt} 次检查没过：{e}")
            if attempt == 2:
                raise
    print(f"报告（{n} 段，逐段检查引用）：\n")
    print(report)
    print(f"\n引用检查：{n} 段通过；每个数字都能在 findings 里找到")


if __name__ == "__main__":
    main()
