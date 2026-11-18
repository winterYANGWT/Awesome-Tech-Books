# 16.2 目录一直在系统提示里；6 个问题里哪几个会激活 Skill，正文进来多少 token。
# mode: run
# expect: refund-handling
# expect: 未激活
import json
import os
from pathlib import Path
from typing import Literal

from harness import Toolbox, run, tool

from ch16.lib.skillkit import catalog, discover

MODEL = os.environ.get("MODEL", "deepseek-flash")
HERE = Path(__file__).resolve().parent

QUESTIONS = [
    ("退款", "收到的《大运河两千年》中间缺了二十多页，订单号 A1659，怎么处理？"),
    ("缺页", "书有缺页，能退吗？"),
    ("金额", "订单 A1680 的典藏版装订脱落，680 元能直接退吗？"),
    ("月报", "给我 2026 年 9 月的运营月报。"),
    ("运费", "买 80 块钱的书要运费吗？"),
    ("推荐书", "有什么适合初中生读的历史书推荐？"),
]


def main() -> None:
    skills, warnings = discover([HERE / "16_3_refund_skill", HERE / "16_5_report_skill"])
    for w in warnings:
        print("提示：", w)
    by_name = {s.name: s for s in skills}
    names = Literal["refund-handling", "monthly-report"]

    # ANCHOR: tool
    @tool
    def activate_skill(name: names) -> dict:
        """读一个 Skill 的正文和捆绑文件列表。name 只能是目录里列出的 Skill。"""
        s = by_name[name]
        return {"skill": s.name, "body": s.body, "files": s.files, "tokens": s.body_tokens}

    prompt = f"""你是青石书店的客服。下面是可以使用的 Skill 目录：

{catalog(skills)}

规则：问题与某个 Skill 的 description 匹配时，先调用 activate_skill(name) 读正文，再按正文回答；
不匹配就不要激活。回答简短，不要复述整份规程。"""
    # ANCHOR_END: tool

    total_tokens = 0
    activated = 0
    for i, (label, question) in enumerate(QUESTIONS, 1):
        messages = [{"role": "system", "content": prompt}, {"role": "user", "content": question}]
        res = run(messages, Toolbox([activate_skill]), model=MODEL, max_turns=3, verbose=False)
        used = [s for s in res.steps if s.name == "activate_skill"]
        if used:
            data = json.loads(used[0].result)
            total_tokens += data["tokens"]
            activated += 1
            print(f"Q{i} {label} → activate_skill({data['skill']})  ← {data['tokens']} token")
        else:
            print(f"Q{i} {label} → 未激活  ← 目录里没有匹配的 description")
    print(f"\n汇总：{activated}/{len(QUESTIONS)} 次激活；目录 {sum(s.catalog_tokens for s in skills)} token 一直在，"
          f"正文累计按需加载 {total_tokens} token")


if __name__ == "__main__":
    main()
