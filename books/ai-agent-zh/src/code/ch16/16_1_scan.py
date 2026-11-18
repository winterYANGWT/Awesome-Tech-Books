# 16.1 扫描两个 Skill 目录：只看目录、正文 token 和捆绑文件。
# mode: run
# llm: none
# expect: 目录
from pathlib import Path

from ch16.lib.skillkit import catalog, discover

HERE = Path(__file__).resolve().parent
ROOTS = [HERE / "16_3_refund_skill", HERE / "16_5_report_skill"]


# ANCHOR: scan
def main() -> None:
    skills, warnings = discover(ROOTS)
    for w in warnings:
        print("提示：", w)
    print(f"Skill 目录（{len(skills)} 个）")
    for s in skills:
        print(f"- {s.name}: {s.description}")
        print(f"  {s.catalog_tokens} token")
    print(f"目录合计 {sum(s.catalog_tokens for s in skills)} token")
    print()
    for s in skills:
        print(f"{s.name}  正文 {s.body_tokens} token，参考文件 {len(s.files)} 个")
        print(f"  SKILL.md（激活时读，{len(s.body.splitlines())} 行）")
        for f in s.files:
            print(f"  {f}（按需）")
        print()
    print("目录（catalog）里只有 name 和 description：")
    print(catalog(skills))
# ANCHOR_END: scan


if __name__ == "__main__":
    main()
