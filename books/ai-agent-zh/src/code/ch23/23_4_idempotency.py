# mode: run
# llm: none
# expect: 无幂等键.*100
# expect: 有幂等键.*50
"""同一笔 50 元部分退款（A1656，实付 183.35）：没有幂等键会退两次，有了只退一次。

`idempotency_key = session_id + ":" + tool_call_id`。工具先查 `idempotency` 表，
有记录就返回上次的结果；没有才真的退款。表建在 `shop.db` 里，不改 `shop` 的代码。
"""
import json
import time

from shop import Store

SESSION = "s-ch23-01"
LIMIT = 3


def open_store():
    store = Store()
    store.db.execute("CREATE TABLE IF NOT EXISTS idempotency(key TEXT PRIMARY KEY, result TEXT)")
    store.db.commit()
    return store


def idempotent_refund(store, key, order_id, amount, reason, *, lose_reply=False):
    row = store.db.execute("SELECT result FROM idempotency WHERE key = ?", (key,)).fetchone()
    if row:
        return json.loads(row[0]), "命中幂等记录"
    out = store.refund(order_id, amount, reason)
    store.db.execute("INSERT INTO idempotency(key, result) VALUES(?, ?)",
                     (key, json.dumps(out, ensure_ascii=False)))
    store.db.commit()
    if lose_reply:                       # 钱已经退了，响应在回程丢了：调用方只看到超时
        raise TimeoutError("退款网关超时（响应没有回来）")
    return out, "新执行"


def with_retry(fn):
    for attempt in range(1, LIMIT + 1):
        try:
            return fn()
        except (TimeoutError, ConnectionError) as e:
            print(f"  第 {attempt} 次：{e}")
            if attempt == LIMIT:
                raise
            time.sleep(0.1 * attempt)    # 退避；真实系统用指数退避加抖动


def main():
    amount = 50.0
    store = open_store()
    print("无幂等键：重试就是再退一次")
    for i in (1, 2):
        out = store.refund("A1656", amount, "缺页")
        print(f"  第 {i} 次 issue_refund(A1656, {amount:.2f}) → refunded_total={out['refunded_total']}")
    refunded = store.order("A1656")["refunded"]
    print(f"无幂等键：两次调用，A1656 累计退款 {refunded:.2f} 元")

    store = open_store()
    key = f"{SESSION}:call_refund_01"
    print(f"有幂等键：{key}")
    lost = lambda: idempotent_refund(store, key, "A1656", amount, "缺页", lose_reply=True)  # noqa: E731
    out, how = with_retry(lost)
    print(f"  第 2 次 → {how}，refunded_total={out['refunded_total']}")
    refunded = store.order("A1656")["refunded"]
    print(f"有幂等键：两次调用，A1656 累计退款 {refunded:.2f} 元")
    print(f"actions 里的 refund 记录 {len([a for a in store.actions() if a['kind'] == 'refund'])} 条")


if __name__ == "__main__":
    main()
