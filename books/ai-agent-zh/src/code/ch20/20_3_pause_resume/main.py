# 20.3 暂停、写 pending.json、进程退出，再由另一个进程批准并恢复。
# mode: run
# expect: 恢复：执行
# expect: 已退款 680.0
"""两个子进程共享 `ch20/shop.db`：ask 在待批准处退出，resume 从 pending.json 接着走。"""
from __future__ import annotations

import json
import pathlib
import subprocess
import sys

from ch20._lib.approvals import (NOW, approval_tools, decide, get, load_pending,
                            pending_record, save_pending)
from harness import ToolError, Toolbox, run
from shop import TODAY, Store

DB = pathlib.Path("ch20/shop.db")
PENDING = pathlib.Path("ch20/pending.json")
NO_THINK = {"thinking": {"type": "disabled"}}
SYSTEM = f"""你是青石书店的客服。今天是 {TODAY}。
订单 A1680 的顾客赵梓萱反馈：精装典藏版十二卷中有三卷装订脱落，要求全额退款 680 元。
已核对：2026-10-29 签收（19 天），装订脱落属于质量问题，60 天内可退全款；但单笔超过 500 元，
必须走主管批准。先申请批准，拿到 approval_id 后再调用 issue_refund_approved 执行。"""


class Pause(Exception):
    def __init__(self, record: dict):
        self.record = record


def ask() -> None:
    DB.unlink(missing_ok=True)
    PENDING.unlink(missing_ok=True)
    store = Store(DB)
    box = approval_tools(store, conversation_id="T13")
    request = box.tools["request_approval"]
    issue = box.tools["issue_refund_approved"]
    messages = [{"role": "system", "content": SYSTEM},
                {"role": "user", "content": "请处理这张订单，按流程走审批。"}]

    # ANCHOR: no_think
    # 第一步固定调用 request_approval（禁用思考才能用指定函数的 tool_choice）。
    run(messages, Toolbox([request]), max_turns=1, extra_body=NO_THINK,
        tool_choice={"type": "function", "function": {"name": "request_approval"}})
    # ANCHOR_END: no_think

    # 第二步固定调用 issue_refund_approved；审批门在这里看到 pending，写文件并退出。
    messages.append({"role": "user", "content": "批准单已创建，现在用它执行退款。"})

    gate = make_gate(store, messages)
    try:
        run(messages, Toolbox([issue]), max_turns=1, extra_body=NO_THINK,
            tool_choice={"type": "function", "function": {"name": "issue_refund_approved"}},
            approve=gate)
    except Pause as paused:
        save_pending(paused.record)
        print(f"PAUSED request_id={paused.record['request_id']}（pending.json 已写，进程退出）")
        return
    raise SystemExit("没有停在待批准处：检查 gate 和 tool_choice")


# ANCHOR: gate
def make_gate(store, messages):
    def gate(tool_obj, args: dict):
        row = get(store, args.get("approval_id", ""))
        if row is None:
            return "没有批准单"
        if row["status"] == "pending":
            tool_call_id = next(
                tc["id"] for tc in messages[-1].get("tool_calls", [])
                if tc["function"]["name"] == tool_obj.name
                and json.loads(tc["function"]["arguments"]) == args)
            raise Pause(pending_record(
                request_id=row["request_id"], tool_name=tool_obj.name, args=args,
                messages=messages, tool_call_id=tool_call_id, expires_at=row["expires_at"]))
        from ch20._lib.approvals import approve_callback
        return approve_callback(store)(tool_obj, args)
    return gate
# ANCHOR_END: gate


# ANCHOR: resume
def resume(approve: bool = False) -> None:
    pending = load_pending()
    store = Store(DB)
    row = get(store, pending["request_id"])
    if approve and row["status"] == "pending":
        decide(store, pending["request_id"], "approved", "主管", "同意按质量问题全款退",
               at=NOW)
    box = approval_tools(store, conversation_id="T13")
    issue = box.tools["issue_refund_approved"]
    args = pending["args"]
    print(f"恢复：执行 {pending['tool']}({args['order_id']}, {args['amount']:.2f})")
    try:
        result = issue.fn(**args)
    except ToolError as exc:
        result = {"status": "错误", "error": str(exc)}
    if result.get("status") == "已退款":
        print(f"工具：已退款 {result['amount']:.2f}，approved_by={result['approved_by']}")
    else:
        print(f"工具：{result.get('status')} {result}")
    messages = pending["messages"]
    messages.append({"role": "tool", "tool_call_id": pending["tool_call_id"],
                     "content": json.dumps(result, ensure_ascii=False)})

    # 用客服的只读工具继续，避免模型再发一次退款。
    from shop.tools import support_tools
    read = support_tools(store, include=("get_order", "search_policies", "escalate_to_supervisor"))
    result_after = run(messages, read, max_turns=3, extra_body=NO_THINK)
    print(f"最终回答：{result_after.answer or '（模型没有给出回答）'}")
    refunds = [a for a in store.actions() if a["kind"] == "refund"]
    print(f"actions：退款 {len(refunds)} 条；"
          f"approval_id={refunds[0]['detail'].get('approval_id') if refunds else '—'}")
# ANCHOR_END: resume


def main() -> None:
    if len(sys.argv) > 1:
        if sys.argv[1] == "ask":
            ask()
        elif sys.argv[1] == "resume":
            resume(approve="--approve" in sys.argv[2:])
        else:
            raise SystemExit(f"未知参数 {sys.argv[1]}")
        return
    print("== ask 进程 ==")
    subprocess.run([sys.executable, __file__, "ask"], check=True)
    print("\n== resume 进程（主管已批准）==")
    subprocess.run([sys.executable, __file__, "resume", "--approve"], check=True)


if __name__ == "__main__":
    main()
