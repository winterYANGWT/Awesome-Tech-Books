# mode: run
# output: varies
# expect: span=model
# expect: span=tool
# expect: span=answer
# expect: 合计
"""T06 缺页退款：在 harness 外面包一层，给模型调用、工具调用和回答各记一行 span。

`harness.run` 的 `Result.steps` 只有工具步骤，看不到模型调用；这里把 `harness._client`
换成一个记账的客户端，工具的 `fn` 也包一层计时。只记长度、hash 和参数键，不记正文。
"""
import hashlib
import json
import os
import pathlib
import time
from collections import Counter
from types import SimpleNamespace

from openai import OpenAI

import harness
from harness import run
from shop import Store, tickets
from shop.tools import SUPPORT_PROMPT, support_tools

MODEL = os.environ.get("MODEL", "deepseek-flash")
TRACE = pathlib.Path("ch23/trace.jsonl")
TRACE_ID = "tr-t06-0001"
SESSION = "S-T06"


def digest(text):
    return hashlib.sha256(text.encode()).hexdigest()[:12]


class Trace:
    def __init__(self, session_id, ticket_id):
        TRACE.parent.mkdir(parents=True, exist_ok=True)
        TRACE.write_text("", encoding="utf-8")
        self.session_id, self.ticket_id = session_id, ticket_id
        self.turn, self.seq, self.last_model, self.rows = 0, 0, None, []

    def add(self, kind, name, *, started, duration_ms, parent=None, tokens=None, detail=None, status="ok"):
        self.seq += 1
        row = {"trace_id": TRACE_ID, "session_id": self.session_id, "ticket_id": self.ticket_id,
               "span_id": f"sp-{self.seq:02d}", "parent_id": parent,
               "turn": self.turn, "kind": kind, "name": name,
               "started_at": time.strftime("%Y-%m-%dT%H:%M:%S", time.localtime(started)),
               "duration_ms": round(duration_ms), "status": status, "tokens": tokens or {}}
        row.update(detail or {})
        with TRACE.open("a", encoding="utf-8") as fh:
            fh.write(json.dumps(row, ensure_ascii=False) + "\n")
        self.rows.append(row)
        return row


class TracedCompletions:
    """包住 `chat.completions.create`：响应回来后记一条 model span。"""

    def __init__(self, inner, trace):
        self.inner, self.trace = inner, trace

    def create(self, **kwargs):
        trace, started = self.trace, time.time()
        trace.turn += 1
        t0 = time.monotonic()
        resp = self.inner.create(**kwargs)
        ms = (time.monotonic() - t0) * 1000
        msg, usage = resp.choices[0].message, resp.usage.model_dump()
        calls = [tc.function.name for tc in (msg.tool_calls or [])]
        row = trace.add("model", kwargs.get("model", MODEL), started=started, duration_ms=ms,
                        tokens={"prompt": usage["prompt_tokens"], "completion": usage["completion_tokens"],
                                "cache_hit": usage.get("prompt_cache_hit_tokens", 0)},
                        detail={"messages": len(kwargs.get("messages", [])),
                                "tools": len(kwargs.get("tools", [])),
                                "finish_reason": resp.choices[0].finish_reason, "tool_calls": calls})
        trace.last_model = row["span_id"]
        print(f"span=model turn={trace.turn} ms={row['duration_ms']} "
              f"tokens={usage['prompt_tokens']}+{usage['completion_tokens']} "
              f"cache_hit={usage.get('prompt_cache_hit_tokens', 0)} "
              f"finish={row['finish_reason']} tools={','.join(calls) or '-'}")
        return resp


def instrument(box, trace):
    """给每个工具的 fn 包一层：先执行，再记一条 tool span（只记字节数和 hash）。"""
    for t in box.tools.values():
        original = t.fn

        def wrapped(*args, _fn=original, _t=t, **kwargs):
            started, t0 = time.time(), time.monotonic()
            args_key = ",".join(sorted(kwargs)) or (f"{len(args)} positional" if args else "")
            try:
                out = _fn(*args, **kwargs)
            except Exception as e:  # 工具错误也要留一行，交给 harness 变成给模型的文字
                trace.add("tool", _t.name, started=started, duration_ms=(time.monotonic() - t0) * 1000,
                          parent=trace.last_model, status="error",
                          detail={"args_keys": args_key, "error": type(e).__name__, "result_bytes": 0})
                print(f"span=tool  turn={trace.turn} {_t.name} error={type(e).__name__}")
                raise
            body = out if isinstance(out, str) else json.dumps(out, ensure_ascii=False)
            row = trace.add("tool", _t.name, started=started, duration_ms=(time.monotonic() - t0) * 1000,
                            parent=trace.last_model,
                            detail={"args_keys": args_key, "result_bytes": len(body.encode()),
                                    "result_hash": digest(body)})
            print(f"span=tool  turn={trace.turn} {_t.name} ms={row['duration_ms']} bytes={row['result_bytes']}")
            return out

        t.fn = wrapped


def traced_run(messages, box, trace):
    """= harness.run，但把模型调用和工具调用都记下来。"""
    instrument(box, trace)
    harness._client = SimpleNamespace(chat=SimpleNamespace(
        completions=TracedCompletions(OpenAI().chat.completions, trace)))
    try:
        result = run(messages, box, model=MODEL, verbose=False)
    finally:
        harness._client = None
    print(f"span=answer turn={result.turns} chars={len(result.answer or '')} stop={result.stop}")
    trace.add("answer", "final_answer", started=time.time(), duration_ms=0, parent=trace.last_model,
              detail={"chars": len(result.answer or ""), "answer_hash": digest(result.answer or ""),
                      "stop": result.stop})
    return result


def main():
    ticket = next(t for t in tickets() if t["id"] == "T06")
    trace, store = Trace(SESSION, ticket["id"]), Store()
    messages = [{"role": "system", "content": SUPPORT_PROMPT},
                {"role": "user", "content": ticket["text"]}]
    result = traced_run(messages, support_tools(store), trace)

    kinds = Counter(r["kind"] for r in trace.rows)
    total_ms = sum(r["duration_ms"] for r in trace.rows if r["kind"] in ("model", "tool"))
    prompt = sum(r["tokens"].get("prompt", 0) for r in trace.rows)
    completion = sum(r["tokens"].get("completion", 0) for r in trace.rows)
    cache_hit = sum(r["tokens"].get("cache_hit", 0) for r in trace.rows)
    print(f"合计 {len(trace.rows)} 条 span：model {kinds['model']} / tool {kinds['tool']} / answer {kinds['answer']}")
    print(f"total_ms={total_ms} prompt={prompt} completion={completion} cache_hit={cache_hit}")
    print(f"trace: ch23/trace.jsonl  stop={result.stop}")


if __name__ == "__main__":
    main()
