# 16.1 坏 frontmatter：缺 description 的被跳过，冒号问题回退后仍加载。
# mode: run
# llm: none
# expect: 跳过 missing-description
# expect: 可用 Skill：1 个
from pathlib import Path

from ch16.lib.skillkit import discover

HERE = Path(__file__).resolve().parent
ROOTS = [HERE / "16_1_bad_skills"]


def main() -> None:
    skills, warnings = discover(ROOTS)
    print(f"扫描 {len(list(ROOTS[0].iterdir()))} 个目录")
    for w in warnings:
        print("提示：", w)
    print(f"可用 Skill：{len(skills)} 个（{', '.join(s.name for s in skills)}）")
    if len(skills) != 1:
        raise SystemExit(f"预期 1 个可用 Skill，实际 {len(skills)} 个：缺 description 的目录应被跳过")


if __name__ == "__main__":
    main()
