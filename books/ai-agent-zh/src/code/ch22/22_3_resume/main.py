# 22.3 父进程跑两段：run 在第 2 轮工具执行前 os._exit(9)，resume 从 session.jsonl 接着走。
# mode: run
# expect: crash at seq=6
# expect: resume from seq=6
# expect: 完成，退款 A1659
"""状态全在文件里：session.jsonl 存事件，shop.db 存业务数据；两个子进程共享它们。"""
from __future__ import annotations

import json
import os
import pathlib
import subprocess
import sys

from harness import fmt_call, short
from openai import OpenAI
from shop import Store
from shop.tools import SUPPORT_PROMPT, support_tools

HERE = pathlib.Path("ch22/22_3_resume")
SESSION = HERE / "session.jsonl"
DB = HERE / "shop.db"
MODEL = os.environ.get("MODEL", "deepseek-flash")
SYSTEM = SUPPORT_PROMPT + "\n每次只调用一个工具，拿到结果再决定下一步。"


class SessionLog:
    def __init__(self, path: pathlib.Path):
        self.path = path
        self.events = ([json.loads(line) for line in path.read_text(encoding="utf-8").splitlines()]
                       if path.exists() else [])

    # ANCHOR: append
    def append(self, kind: str, **payload) -> dict:
        e = {"seq": len(self.events) + 1, "type": kind, "payload": payload,
             "created_at": "2026-11-17T11:00:00+08:00"}
        self.events.append(e)
        self.path.parent.mkdir(parents=True, exist_ok=True)
        with self.path.open("a", encoding="utf-8") as f:
            f.write(json.dumps(e, ensure_ascii=False) + "\n")
            f.flush()
            os.fsync(f.fileno())          # 追加一条就落盘，崩溃时不靠内存里的缓冲
        return e
    # ANCHOR_END: append

    def messages(self) -> list[dict]:
        out: list[dict] = []
        for e in self.events:
            p, kind = e["payload"], e["type"]
            if kind == "session_started":
                out.append({"role": "system", "content": p["system"]})
            elif kind == "user":
                out.append({"role": "user", "content": p["content"]})
            elif kind == "assistant":
                m = {"role": "assistant", "content": p["content"]}
                if p.get("tool_calls"):
                    m["tool_calls"] = p["tool_calls"]
                if p.get("reasoning_content"):
                    m["reasoning_content"] = p["reasoning_content"]
                out.append(m)
            elif kind == "tool_result":
                out.append({"role": "tool", "tool_call_id": p["tool_call_id"], "content": p["content"]})
            elif kind == "answer":
                m = {"role": "assistant", "content": p["content"]}
                if p.get("reasoning_content"):
                    m["reasoning_content"] = p["reasoning_content"]
                out.append(m)
        return out


def execute(session: SessionLog, box, call: dict) -> None:
    name = call["function"]["name"]
    args = json.loads(call["function"]["arguments"] or "{}")
    sid = session.events[0]["payload"].get("session_id", "")
    session.append("tool_call", tool_call_id=call["id"], name=name, arguments=args,
                   idempotency_key=f"{sid}:{call['id']}")
    out = box.call(name, call["function"]["arguments"])
    session.append("tool_result", tool_call_id=call["id"], content=out)
    print(f"  → {fmt_call(name, args)}  ← {short(out)}", flush=True)


def turn(session: SessionLog, box, number: int, crash: bool) -> bool:
    resp = OpenAI().chat.completions.create(model=MODEL, messages=session.messages(), tools=box.schemas())
    msg = resp.choices[0].message
    print(f"第 {number} 轮：模型返回 {len(msg.tool_calls or [])} 个 tool_call", flush=True)
    entry = {"content": msg.content or ""}
    reasoning = getattr(msg, "reasoning_content", None)
    if reasoning:
        entry["reasoning_content"] = reasoning
    if not msg.tool_calls:
        session.append("answer", **entry)       # 最终回答只记一条，不要和 assistant 重复
        return False
    entry["tool_calls"] = [tc.model_dump(include={"id", "type", "function"}) for tc in msg.tool_calls]
    session.append("assistant", **entry)
    if crash:
        print(f"crash at seq={session.events[-1]['seq']}（工具还没执行，进程退出）", flush=True)
        os._exit(9)
    for tc in msg.tool_calls:
        execute(session, box, tc.model_dump(include={"id", "type", "function"}))
    return True


def run() -> None:
    store, log = Store(DB), SessionLog(SESSION)
    log.append("session_started", session_id="S-T06", system=SYSTEM, model=MODEL)
    log.append("user", content="订单 A1659 的《大运河两千年》缺了二十多页，请处理。")
    box = support_tools(store, include=("get_order", "issue_refund"))
    for n in (1, 2):
        if not turn(log, box, n, crash=(n == 2)):
            break


def resume() -> None:
    log = SessionLog(SESSION)
    if not log.events:
        raise SystemExit("没有 session.jsonl，先跑 run")
    print(f"resume from seq={log.events[-1]['seq']}（wake + getSession）", flush=True)
    store = Store(DB)
    box = support_tools(store, include=("get_order", "issue_refund"))
    # ANCHOR: pending
    # 示例只处理第一个窗口：找 assistant 里还没有 tool_result 的调用。
    done = {e["payload"]["tool_call_id"] for e in log.events if e["type"] == "tool_result"}
    intents = {e["payload"]["tool_call_id"] for e in log.events if e["type"] == "tool_call"}
    pending = [tc for e in log.events if e["type"] == "assistant"
               for tc in e["payload"].get("tool_calls", []) if tc["id"] not in done]
    for call in pending:
        if call["id"] in intents:
            raise SystemExit(f"{call['id']} 已有 tool_call 意图但没有结果；先查幂等记录（第 23 章）")
        execute(log, box, call)
    # ANCHOR_END: pending
    number = sum(e["type"] == "assistant" for e in log.events) + 1
    while turn(log, box, number, crash=False):
        number += 1
    order = store.order("A1659")
    print(f"完成，退款 A1659：{order['refunded']:.0f} 元；actions {len(store.actions())} 条", flush=True)
    log.append("session_ended", reason="answer")


def main() -> None:
    if len(sys.argv) > 1:
        {"run": run, "resume": resume}[sys.argv[1]]()
        return
    HERE.mkdir(parents=True, exist_ok=True)
    SESSION.unlink(missing_ok=True)
    DB.unlink(missing_ok=True)
    print("== run 进程 ==")
    first = subprocess.run([sys.executable, __file__, "run"])
    print(f"\nrun 进程退出码 {first.returncode}")
    print("\n== resume 进程 ==")
    subprocess.run([sys.executable, __file__, "resume"], check=True)


if __name__ == "__main__":
    main()
