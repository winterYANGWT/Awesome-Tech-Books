"""第 15 章示例共用：token 估算、压缩、工具结果清理、NOTES.md 读写。

估算只用来在调用前看方向；正文里的账单一律以 `usage.prompt_tokens` 为准。
"""
from __future__ import annotations

import copy
import json
import os
import pathlib
import tempfile


def _is_cjk(ch: str) -> bool:
    return "\u3400" <= ch <= "\u9fff" or "\u3000" <= ch <= "\u303f" or "\uff00" <= ch <= "\uffef"


def text_tokens(text: str) -> int:
    text = str(text)
    cjk = sum(1 for ch in text if _is_cjk(ch))
    return int(cjk * 0.75 + (len(text) - cjk) * 0.4 + 0.999)


def message_tokens(m: dict) -> int:
    n = text_tokens(m.get("content") or "")
    if m.get("tool_calls"):
        n += len(json.dumps(m["tool_calls"], ensure_ascii=False)) // 4
    return n


def tools_tokens(tools: list | None) -> int:
    return len(json.dumps(tools or [], ensure_ascii=False)) // 4


def categories(messages: list, tools: list | None = None) -> dict[str, int]:
    out = {"系统提示": 0, "工具定义": tools_tokens(tools), "对话": 0, "工具结果": 0}
    for m in messages:
        if m["role"] == "system":
            out["系统提示"] += message_tokens(m)
        elif m["role"] == "tool":
            out["工具结果"] += message_tokens(m)
        else:
            out["对话"] += message_tokens(m)
    return out


def count_request(messages: list, tools: list | None = None) -> int:
    return sum(categories(messages, tools).values())


def check_pairs(messages: list) -> None:
    pending: list[str] = []
    for m in messages:
        if m["role"] == "assistant" and m.get("tool_calls"):
            pending = [tc["id"] for tc in m["tool_calls"]]
        elif m["role"] == "tool":
            if not pending or m.get("tool_call_id") not in pending:
                raise ValueError(f"tool 消息 {m.get('tool_call_id')} 没有配对的 tool_calls")
            pending.remove(m["tool_call_id"])
        elif pending:
            raise ValueError(f"tool_calls 里的 {pending[0]} 没有对应的 tool 消息")


# ---- 压缩 -----------------------------------------------------------------

SUMMARY_SYSTEM = """你是客服会话的压缩器。把对话压缩成一份可以继续工作的摘要。
必须保留（原文里有就写，没有就写“未确认”）：
- 订单号、金额（写成两位小数，例如 82.00）、退款去向；
- 顾客的约束和偏好：退款方式、联系方式、时间要求；
- 已完成的动作和结果；
- 未决事项和下一步；
- 引用过的政策条款标题。
如果对话里已经有一段旧摘要，把新信息合并进去，不要丢掉旧摘要里的字段。
只输出摘要正文，300 字以内。"""

LOSSY_SYSTEM = """把对话压缩成一份摘要，只保留订单号、金额和退款原因。
其他内容一律删掉，不要写退款去向、顾客偏好和未决事项。
只输出摘要正文，100 字以内。"""


def render(messages: list) -> str:
    lines = []
    for m in messages:
        role = m["role"]
        body = " ".join(str(m.get("content") or "").split())
        if m.get("tool_calls"):
            names = ", ".join(tc["function"]["name"] for tc in m["tool_calls"])
            body = (body + f" [调用 {names}]").strip()
        lines.append(f"{role}: {body}")
    return "\n".join(lines)


def summarize(client, messages: list, model: str, system: str = SUMMARY_SYSTEM) -> tuple[str, dict]:
    """一次摘要调用；返回摘要文本和这次调用的 usage。"""
    resp = client.chat.completions.create(
        model=model,
        messages=[{"role": "system", "content": system},
                  {"role": "user", "content": render(messages)}],
        max_tokens=600,
        extra_body={"thinking": {"type": "disabled"}},
    )
    return resp.choices[0].message.content.strip(), resp.usage.model_dump()


def compact(messages: list, summary: str, keep_last: int = 4) -> list:
    """系统提示 + 摘要 + 最近几条原文；最近的工具调用和结果要成对保留。"""
    system = messages[0]["content"].split("\n\n## 之前对话的摘要")[0]
    new_system = {"role": "system",
                  "content": system + "\n\n## 之前对话的摘要\n" + summary}
    i = max(1, len(messages) - keep_last)
    while i > 1 and messages[i]["role"] == "tool":
        i -= 1
    return [new_system] + copy.deepcopy(messages[i:])


# ---- 清工具结果 -------------------------------------------------------------

def _amount(data: dict) -> str | None:
    for key in ("total_paid", "amount", "refunded_total"):
        if key in data and data[key] is not None:
            return f"{float(data[key]):.2f}"
    return None


def tool_stub(name: str, content: str) -> str:
    """把旧工具结果换成一两行 stub：保留 ID、金额、状态，必要时保留错误。"""
    try:
        data = json.loads(content)
    except (TypeError, json.JSONDecodeError):
        flat = " ".join(str(content).split())
        return f"[已清理 {name}：{flat[:40]}]"
    if isinstance(data, dict) and "error" in data:
        return f"[已清理 {name}：错误 {str(data['error'])[:40]}]"
    if isinstance(data, dict) and "order_id" in data:
        money = _amount(data)
        state = data.get("status", "—")
        return f"[已清理 {name}({data['order_id']})：金额 {money or '—'}，状态 {state}]"
    if isinstance(data, dict) and "ticket" in data:
        return f"[已清理 {name}：工单 {data['ticket']}，状态 {data.get('status', '—')}]"
    if isinstance(data, dict) and "recent_orders" in data:
        return f"[已清理 {name}({data.get('customer_id')})：最近 {len(data['recent_orders'])} 张订单]"
    if isinstance(data, list) and data and isinstance(data[0], dict):
        first = data[0]
        if "section" in first:
            titles = "；".join(str(x["section"]) for x in data[:3])
            return f"[已清理 {name}：政策条款 {titles}]"
        if "order_id" in first:
            return f"[已清理 {name}：{len(data)} 张旧订单，{first['order_id']}–{data[-1]['order_id']}]"
        if "stars" in first:
            return f"[已清理 {name}：{len(data)} 条评论]"
    if isinstance(data, list):
        return f"[已清理 {name}：{len(data)} 条记录]"
    return f"[已清理 {name}：{str(data)[:40]}]"


def clear_tool_results(messages: list, keep_last: int = 2) -> tuple[list, int]:
    """只换旧 tool 消息的 content，不删消息；返回 (新列表, 清了几条)。"""
    msgs = copy.deepcopy(messages)
    names = {}
    for m in msgs:
        for tc in m.get("tool_calls") or []:
            names[tc["id"]] = tc["function"]["name"]
    tool_idx = [i for i, m in enumerate(msgs) if m["role"] == "tool"]
    old = tool_idx[:-keep_last] if keep_last else tool_idx
    for i in old:
        name = names.get(msgs[i].get("tool_call_id"), "tool")
        msgs[i]["content"] = tool_stub(name, msgs[i]["content"])
    return msgs, len(old)


# ---- NOTES.md -------------------------------------------------------------

NOTES = pathlib.Path("NOTES.md")
NOTE_SECTIONS = ("## 目标", "## 已完成", "## 待办", "## 关键事实", "## 来源")


class NotesError(Exception):
    """笔记格式不对：消息写给模型看，列出缺哪一节。"""


def read_notes() -> str:
    return NOTES.read_text(encoding="utf-8") if NOTES.exists() else "（还没有笔记）"


def write_notes(text: str) -> str:
    missing = [s for s in NOTE_SECTIONS if s not in text]
    if missing:
        raise NotesError("笔记必须包含五节：" + "、".join(missing))
    if text_tokens(text) > 500:
        raise NotesError(f"笔记约 {text_tokens(text)} token，超过 500 token 的上限")
    fd, tmp = tempfile.mkstemp(dir=".", prefix=".NOTES.", suffix=".tmp")
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as fh:
            fh.write(text.rstrip() + "\n")
        os.replace(tmp, NOTES)
    finally:
        if os.path.exists(tmp):
            os.unlink(tmp)
    return f"已写入 {NOTES.name}（约 {text_tokens(text)} token）"
