"""第 20 章的审批单、凭单退款和 `pending.json`。不改 `shop` 的代码和 schema。

审批单存在业务库的 `approvals` 表里（`CREATE TABLE IF NOT EXISTS`），退款本身仍走
`Store.refund`，所以 `actions` 表里会留下 `kind="refund"` 和 `approved_by`。
"""
from __future__ import annotations

import copy
import json
import pathlib
from datetime import datetime, timedelta
from typing import Annotated

from harness import ToolError, Toolbox, tool
from shop import Store

# 示例里的“现在”是固定的，保证录制回放一致；真实系统用 datetime.now()。
NOW = datetime.fromisoformat("2026-11-17T09:00:00+08:00")
TTL = timedelta(hours=4)
PENDING = pathlib.Path("ch20/pending.json")

# 审批人的额度上限（元）：客服 500、主管 2000、财务更大。
LEVEL_LIMIT = {"客服": 500.0, "主管": 2000.0, "财务": 100000.0}

SCHEMA = """
CREATE TABLE IF NOT EXISTS approvals(
    request_id      TEXT PRIMARY KEY,
    conversation_id TEXT NOT NULL,
    tool            TEXT NOT NULL,
    args            TEXT NOT NULL,
    status          TEXT NOT NULL,       -- pending / approved / rejected / expired
    decision        TEXT,                -- approved / rejected / expired
    approver        TEXT,
    reason          TEXT,
    created_at      TEXT NOT NULL,
    expires_at      TEXT NOT NULL,
    decided_at      TEXT,
    executed_at     TEXT
);
"""


def init(store: Store) -> None:
    store.db.executescript(SCHEMA)
    store.db.commit()


def iso(value: datetime) -> str:
    return value.isoformat(timespec="seconds")


def get(store: Store, request_id: str) -> dict | None:
    init(store)
    rows = store.query("SELECT * FROM approvals WHERE request_id = ?", request_id)
    if not rows:
        return None
    row = rows[0]
    row["args"] = json.loads(row["args"])
    return row


def new_request(store: Store, order_id: str, amount: float, reason: str, *,
                conversation_id: str = "T13", request_id: str | None = None,
                tool_name: str = "issue_refund_approved", created_at: datetime = NOW,
                ttl: timedelta = TTL) -> dict:
    """生成一张待批准单。`approval_id` 就是 `request_id`。"""
    init(store)
    if request_id is None:
        n = store.query("SELECT COUNT(*) AS n FROM approvals")[0]["n"] + 1
        request_id = f"R-{n:04d}"
    args = {"order_id": order_id, "amount": round(float(amount), 2), "reason": reason}
    store.db.execute(
        "INSERT INTO approvals(request_id, conversation_id, tool, args, status, created_at, expires_at) "
        "VALUES(?, ?, ?, ?, 'pending', ?, ?)",
        (request_id, conversation_id, tool_name, json.dumps(args, ensure_ascii=False),
         iso(created_at), iso(created_at + ttl)))
    store.db.commit()
    return {"approval_id": request_id, "status": "pending",
            "expires_at": iso(created_at + ttl)}


def decide(store: Store, request_id: str, decision: str, approver: str,
           reason: str = "", at: datetime = NOW) -> dict:
    """批准或拒绝；超过 `expires_at` 的单一律记为 expired，不再接受批准。"""
    row = get(store, request_id)
    if row is None:
        raise ToolError(f"没有批准单 {request_id}")
    if decision not in ("approved", "rejected"):
        raise ToolError(f"decision 只能是 approved 或 rejected：{decision!r}")
    if row["status"] != "pending":
        return row
    if at > datetime.fromisoformat(row["expires_at"]):
        status, reason = "expired", "审批超时，已过 expires_at"
    else:
        status = decision
    store.db.execute(
        "UPDATE approvals SET status = ?, decision = ?, approver = ?, reason = ?, decided_at = ? "
        "WHERE request_id = ?",
        (status, status, approver, reason, iso(at), request_id))
    store.db.commit()
    return get(store, request_id)


def _tag_last_action(store: Store, request_id: str, approved_by: str) -> None:
    """在 shop 写下的 actions.detail 里补上 approval_id，便于审计按单号查重。"""
    row = store.query("SELECT seq, order_id, detail FROM actions ORDER BY seq DESC LIMIT 1")[0]
    detail = json.loads(row["detail"])
    detail["approval_id"] = request_id
    detail["order_id"] = row["order_id"]
    detail.setdefault("approved_by", approved_by)
    store.db.execute("UPDATE actions SET detail = ? WHERE seq = ?",
                     (json.dumps(detail, ensure_ascii=False), row["seq"]))
    store.db.commit()


def execution(store: Store, request_id: str) -> dict | None:
    for action in store.actions():
        if action["kind"] == "refund" and action["detail"].get("approval_id") == request_id:
            return action
    return None


# ANCHOR: execute
def execute_approved_refund(store: Store, order_id: str, amount: float, reason: str,
                            approval_id: str) -> dict:
    """凭已批准的批准单退款。检查状态、参数、过期和幂等，任何一项不对都不执行。"""
    row = get(store, approval_id)
    if row is None:
        raise ToolError(f"没有批准单 {approval_id}，不能退款")
    if row["status"] != "approved":
        raise ToolError(f"批准单 {approval_id} 的状态是 {row['status']}，不能执行"
                        + (f"（{row['reason']}）" if row["reason"] else ""))
    if datetime.fromisoformat(row["expires_at"]) < NOW:
        raise ToolError(f"批准单 {approval_id} 已于 {row['expires_at']} 过期")
    approved = row["args"]
    if approved["order_id"] != order_id or abs(approved["amount"] - round(float(amount), 2)) > 1e-9:
        raise ToolError(f"调用参数和批准单不一致：批准的是 {approved['order_id']} "
                        f"{approved['amount']:.2f} 元，收到 {order_id} {float(amount):.2f} 元")
    if approved["reason"] != reason:
        raise ToolError(f"退款原因和批准单不一致：批准的是“{approved['reason']}”")
    done = execution(store, approval_id)
    if done is not None:
        return {"status": "已执行过", "order_id": order_id, "amount": done["detail"]["amount"],
                "approval_id": approval_id, "approved_by": row["approver"]}
    result = store.refund(order_id, amount, reason, approved_by=row["approver"])
    _tag_last_action(store, approval_id, row["approver"])
    store.db.execute("UPDATE approvals SET executed_at = ? WHERE request_id = ?",
                     (iso(NOW), approval_id))
    store.db.commit()
    return {**result, "approval_id": approval_id, "approved_by": row["approver"]}
# ANCHOR_END: execute


# ANCHOR: gate
def approve_callback(store: Store, now: datetime = NOW):
    """给 `harness.run(approve=...)` 用：只校验批准单和参数，不再问人。"""
    def gate(tool_obj, args: dict) -> bool | str:
        request_id = args.get("approval_id")
        if not request_id:
            return "缺少 approval_id，先调用 request_approval"
        row = get(store, request_id)
        if row is None:
            return f"没有批准单 {request_id}"
        if row["status"] != "approved":
            return f"批准单 {request_id} 状态是 {row['status']}：{row['reason'] or '未批准'}"
        if now > datetime.fromisoformat(row["expires_at"]):
            return f"批准单 {request_id} 已过期"
        approved = row["args"]
        if approved["order_id"] != args.get("order_id"):
            return f"订单和批准单不一致：批准的是 {approved['order_id']}"
        if abs(approved["amount"] - round(float(args.get("amount", -1)), 2)) > 1e-9:
            return f"金额和批准单不一致：批准的是 {approved['amount']:.2f} 元"
        if approved["reason"] != args.get("reason"):
            return "退款原因和批准单不一致"
        return True
    return gate
# ANCHOR_END: gate


def approval_tools(store: Store, *, conversation_id: str = "T13") -> Toolbox:
    """返回 `request_approval` 和 `issue_refund_approved` 两个工具。"""
    init(store)

    @tool
    def request_approval(order_id: Annotated[str, "订单号，例如 A1680"],
                         amount: Annotated[float, "申请的退款金额（元）"],
                         reason: Annotated[str, "退款原因和政策依据"]) -> dict:
        """为超过 500 元的退款申请主管批准。生成批准单并记一条 escalate，不退款。
        返回 approval_id；拿到后调用 issue_refund_approved 执行。"""
        result = new_request(store, order_id, amount, reason, conversation_id=conversation_id)
        store.escalate(order_id, f"申请退款 {amount:.2f} 元：{reason}（{result['approval_id']}）")
        return result

    @tool(risky=True)
    def issue_refund_approved(order_id: Annotated[str, "订单号"],
                              amount: Annotated[float, "退款金额（元）"],
                              reason: Annotated[str, "退款原因，必须和批准单一致"],
                              approval_id: Annotated[str, "request_approval 返回的批准单号"]) -> dict:
        """凭已批准的批准单退款。没有批准单、状态不对、参数不一致或已执行过都会拒绝。
        先调用 request_approval 拿 approval_id，批准后再调用这个工具。"""
        return execute_approved_refund(store, order_id, amount, reason, approval_id)

    return Toolbox([request_approval, issue_refund_approved])


def save_pending(record: dict) -> None:
    PENDING.parent.mkdir(parents=True, exist_ok=True)
    PENDING.write_text(json.dumps(record, ensure_ascii=False, indent=2), encoding="utf-8")


def load_pending() -> dict:
    return json.loads(PENDING.read_text(encoding="utf-8"))


def pending_record(*, request_id: str, tool_name: str, args: dict, messages: list,
                   tool_call_id: str, expires_at: str, conversation_id: str = "T13") -> dict:
    """待批准记录：恢复时照着它执行，不重新问模型。"""
    return {
        "request_id": request_id,
        "conversation_id": conversation_id,
        "tool": tool_name,
        "args": copy.deepcopy(args),
        "messages": copy.deepcopy(messages),
        "tool_call_id": tool_call_id,
        "created_at": iso(NOW),
        "expires_at": expires_at,
        "status": "pending",
    }
