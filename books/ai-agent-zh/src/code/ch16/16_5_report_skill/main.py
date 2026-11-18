# 16.5 月度运营报告 Skill：先跑 report.py，再按模板写五节。
# mode: run
# expect: 2026-09
# expect: 23.0%
# timeout: 300
import json
import os
import subprocess
import sys
from pathlib import Path

from harness import ToolError, Toolbox, run, tool

from ch16.lib.skillkit import load_skill

MODEL = os.environ.get("MODEL", "deepseek-flash")
HERE = Path(__file__).resolve().parent
SKILL_DIR = HERE / "monthly-report"
SCRIPT = SKILL_DIR / "scripts" / "report.py"


@tool
def run_report(month: str) -> dict:
    """运行 monthly-report Skill 里的 report.py，返回一行 CSV。month 用 YYYY-MM，例如 2026-09。"""
    cp = subprocess.run([sys.executable, str(SCRIPT), "--month", month],
                        cwd=SKILL_DIR, capture_output=True, text=True, timeout=30)
    if cp.returncode != 0:
        raise ToolError(f"report.py 退出码 {cp.returncode}：{cp.stderr.strip()}")
    return {"month": month, "csv": cp.stdout.strip()}


def main() -> None:
    skill, warnings = load_skill(SKILL_DIR)
    assert skill is not None, warnings
    question = "给我 2026 年 9 月的运营月报。"
    system = ("你是青石书店的运营助理。今天是 2026-11-17。下面是已经激活的月度报告 Skill，按它执行。"
              "先调用 run_report 拿 CSV，再按模板写五节；不要凭印象写数字；建议里的完成时间要晚于今天。\n\n"
              + skill.body)
    messages = [{"role": "system", "content": system}, {"role": "user", "content": question}]
    res = run(messages, Toolbox([run_report]), model=MODEL, max_turns=4, verbose=False)

    calls = [s for s in res.steps if s.name == "run_report"]
    print(f"问题：{question}")
    if calls:
        print(f"月份：{calls[0].arguments.get('month')}")
        print("脚本输出（CSV）：")
        print(json.loads(calls[0].result)["csv"])
    else:
        print("没有调用 run_report")
    print("\n报告：")
    print(res.answer)


if __name__ == "__main__":
    main()
