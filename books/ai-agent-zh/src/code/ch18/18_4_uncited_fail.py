# 18.4 反例：提示要求“有说服力、不用列数字”，检查器拒绝无来源的因果句。
# mode: fail
# expect: UncitedClaim
import os

from openai import OpenAI

from ch18.lib.common import check_citations
from ch18.lib.fixtures import FINDINGS, all_findings

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")
QUESTION = "2026 年 Q3 退货率为什么上升，Q4 为什么回落？"

# 这个提示只要求“像一份结论”，不要求数字和来源——失败是设计出来的
PROMPT = """你是运营总监，写 5 段短评回答下面的问题，要有说服力。
第一段只有一句话，照抄：“退货政策变宽松是主因。”不要数字，不要来源，第一段后面空一行。
其余四段也只写判断和态度，不要列数字，不要标注来源。"""


def main() -> None:
    evidence = "\n".join(f"- {f['claim']} 证据：{f['evidence']} 来源：{f['source']}"
                         for f in all_findings())
    resp = client.chat.completions.create(
        model=MODEL,
        messages=[{"role": "system", "content": PROMPT},
                  {"role": "user", "content": f"问题：{QUESTION}\n\nfindings：\n{evidence}"}],
        max_tokens=900, extra_body={"thinking": {"type": "disabled"}})
    report = (resp.choices[0].message.content or "").strip()
    print("报告：\n")
    print(report)
    print("\n逐段检查数字和来源……")
    check_citations(report, all_findings())          # 第一段就会抛 UncitedClaim


if __name__ == "__main__":
    main()
