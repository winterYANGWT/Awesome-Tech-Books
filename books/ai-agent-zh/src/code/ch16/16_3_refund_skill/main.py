# 16.3 用退款 Skill 处理 4 张工单：T06、T09、T13、T14。
# mode: run
# expect: 82.00
# expect: 680.00
# timeout: 300
import json
import os
import subprocess
import sys
from pathlib import Path

from harness import ToolError, run, tool
from shop import Store
from shop.tools import support_tools

from ch16.lib.skillkit import load_skill

MODEL = os.environ.get("MODEL", "deepseek-flash")
HERE = Path(__file__).resolve().parent
SKILL_DIR = HERE / "refund-handling"
SCRIPT = SKILL_DIR / "scripts" / "refund.py"

TICKETS = [
    ("T06", "A1659", "收到的《大运河两千年》中间缺了二十多页，订单号 A1659，怎么处理？"),
    ("T09", "A1660", "《北方的河》买重复了，订单 A1660，想退掉一本。"),
    ("T13", "A1680", "订单 A1680 的典藏版十二卷里有三卷装订脱落，要求全额退款。"),
    ("T14", "A1649", "《小学语文阅读训练》拆开做了两页发现不是孩子要的，订单 A1649，退一下。"),
]

store = Store()
LAST: dict[str, dict] = {}
CALLS = {"n": 0}


@tool
def run_refund_script(order_id: str, reason: str, opened: bool = False, book: str = "") -> dict:
    """运行 Skill 里的 refund.py 计算金额。reason 用 quality 或 changed_mind；教辅拆封时 opened=true。"""
    cmd = [sys.executable, str(SCRIPT), "--order", order_id, "--reason", reason]
    if opened:
        cmd.append("--opened")
    if book:
        cmd += ["--book", book]
    CALLS["n"] += 1
    cp = subprocess.run(cmd, cwd=SKILL_DIR, capture_output=True, text=True, timeout=30)
    if cp.returncode != 0:
        raise ToolError(f"refund.py 退出码 {cp.returncode}：{cp.stderr.strip()}")
    data = json.loads(cp.stdout)
    LAST[order_id] = data
    return data


def main() -> None:
    skill, warnings = load_skill(SKILL_DIR)
    assert skill is not None, warnings
    box = support_tools(store, include=("get_order", "issue_refund", "escalate_to_supervisor"))
    box.add(run_refund_script)
    system = ("你是青石书店的客服。下面是已经激活的退款 Skill，按它处理工单。"
              "先用 get_order 核对订单，再用 run_refund_script 计算金额；不要自己心算。\n\n" + skill.body)

    counts = {"refund": 0, "escalate": 0, "refuse": 0}
    for tid, oid, text in TICKETS:
        messages = [{"role": "system", "content": system}, {"role": "user", "content": text}]
        run(messages, box, model=MODEL, max_turns=6, verbose=False)
        actions = [a for a in store.actions() if a["order_id"] == oid]
        kinds = {a["kind"] for a in actions}
        script = LAST.get(oid)
        amount = f"{script['amount']:.2f}" if script else "0.00"
        if "refund" in kinds:
            counts["refund"] += 1
            print(f"{tid} 脚本 amount={amount} needs_approval={script['needs_approval'] if script else '?'}"
                  f" → issue_refund → 已退款")
        elif "escalate" in kinds:
            counts["escalate"] += 1
            print(f"{tid} 脚本 amount={amount} needs_approval={script['needs_approval'] if script else '?'}"
                  f" → escalate_to_supervisor → 已提交主管")
        else:
            counts["refuse"] += 1
            why = script["basis"] if script and not script["refundable"] else "教辅拆封，非质量问题不退款"
            print(f"{tid} 未退款：{why}")
    print(f"\n脚本调用 {CALLS['n']} 次；退款 {counts['refund']} 笔，提交主管 {counts['escalate']} 笔，"
          f"拒绝 {counts['refuse']} 笔")


if __name__ == "__main__":
    main()
