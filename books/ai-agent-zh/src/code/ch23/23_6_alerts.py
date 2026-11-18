# mode: run
# llm: none
# expect: ALERT p95
# expect: ALERT cache_hit
# expect: ALERT
"""从一段 trace 算值班指标：p50/p95 延迟、错误率、缓存命中率、每会话 token，再和基线比。

样本内置，不读 23.1 写下的 `ch23/trace.jsonl`：先跑过 23.1 的人再跑这里，也应该看到同一组坏样本。
"""
import math
from collections import defaultdict

BASELINE = {"p95_ms": 5000, "error_rate": 0.02, "cache_hit": 0.70, "tokens_per_session": 8000}

SAMPLE = [
    {"trace_id": "tr-builtin-01", "span_id": "sp-01", "kind": "model", "name": "deepseek-flash",
     "duration_ms": 820, "status": "ok", "tokens": {"prompt": 1200, "completion": 80, "cache_hit": 900}},
    {"trace_id": "tr-builtin-01", "span_id": "sp-02", "kind": "tool", "name": "get_order",
     "duration_ms": 12, "status": "ok", "tokens": {}},
    {"trace_id": "tr-builtin-01", "span_id": "sp-03", "kind": "model", "name": "deepseek-flash",
     "duration_ms": 1450, "status": "ok", "tokens": {"prompt": 1600, "completion": 90, "cache_hit": 400}},
    {"trace_id": "tr-builtin-01", "span_id": "sp-04", "kind": "tool", "name": "search_policies",
     "duration_ms": 18, "status": "ok", "tokens": {}},
    {"trace_id": "tr-builtin-01", "span_id": "sp-05", "kind": "model", "name": "deepseek-flash",
     "duration_ms": 24000, "status": "ok", "tokens": {"prompt": 2100, "completion": 120, "cache_hit": 100}},
    {"trace_id": "tr-builtin-01", "span_id": "sp-06", "kind": "tool", "name": "issue_refund",
     "duration_ms": 15, "status": "error", "tokens": {}},
    {"trace_id": "tr-builtin-01", "span_id": "sp-07", "kind": "model", "name": "deepseek-flash",
     "duration_ms": 980, "status": "ok", "tokens": {"prompt": 2400, "completion": 100, "cache_hit": 200}},
]


def percentile(values, p):
    ordered = sorted(values)
    return ordered[max(0, math.ceil(p * len(ordered)) - 1)]


def main():
    spans = [s for s in SAMPLE if s["kind"] in ("model", "tool")]
    print(f"数据：内置样例，{len(spans)} 条 span")

    durations = [s["duration_ms"] for s in spans]
    errors = [s for s in spans if s["status"] == "error"]
    prompt = sum(s["tokens"].get("prompt", 0) for s in spans)
    completion = sum(s["tokens"].get("completion", 0) for s in spans)
    hit = sum(s["tokens"].get("cache_hit", 0) for s in spans)

    per_session = defaultdict(lambda: {"prompt": 0, "completion": 0})
    for s in spans:
        per_session[s["trace_id"]]["prompt"] += s["tokens"].get("prompt", 0)
        per_session[s["trace_id"]]["completion"] += s["tokens"].get("completion", 0)
    session_tokens = {tid: t["prompt"] + t["completion"] for tid, t in per_session.items()}
    worst_sid, worst_tokens = max(session_tokens.items(), key=lambda item: item[1])

    p50, p95 = percentile(durations, 0.50), percentile(durations, 0.95)
    error_rate = len(errors) / len(spans)
    hit_rate = hit / prompt if prompt else 0

    print(f"p50={p50}ms  p95={p95}ms（基线 {BASELINE['p95_ms']}ms，超过 2 倍告警）")
    print(f"错误率 {error_rate:.1%}（{len(errors)}/{len(spans)}，阈值 5%）")
    print(f"缓存命中率 {hit_rate:.1%}（{hit:,}/{prompt:,}，基线 {BASELINE['cache_hit']:.0%}，掉一半告警）")
    print(f"每会话 token {worst_tokens:,}（{worst_sid}，预算 {BASELINE['tokens_per_session']:,}）")

    if p95 > 2 * BASELINE["p95_ms"]:
        print(f"ALERT p95：{p95}ms > {2 * BASELINE['p95_ms']}ms")
    else:
        print(f"OK p95：{p95}ms")
    if error_rate > 0.05:
        print(f"ALERT error_rate：{error_rate:.1%} > 5%")
    else:
        print(f"OK error_rate：{error_rate:.1%}")
    if hit_rate < BASELINE["cache_hit"] / 2:
        print(f"ALERT cache_hit：{hit_rate:.1%} < {BASELINE['cache_hit'] / 2:.0%}")
    else:
        print(f"OK cache_hit：{hit_rate:.1%}")
    if worst_tokens > BASELINE["tokens_per_session"]:
        print(f"ALERT tokens：{worst_tokens:,} > {BASELINE['tokens_per_session']:,}")
    else:
        print(f"OK tokens：{worst_tokens:,} <= {BASELINE['tokens_per_session']:,}")

    slow = max(spans, key=lambda s: s["duration_ms"])
    if errors:
        gone = errors[0]
        print(f"按 trace_id 查：{gone['trace_id']} → {gone['span_id']}（{gone['name']}，{gone['status']}）")
    print(f"最慢的一步：{slow['span_id']} {slow['name']} {slow['duration_ms']}ms")


if __name__ == "__main__":
    main()
