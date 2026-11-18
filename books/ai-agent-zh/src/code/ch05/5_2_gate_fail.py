# mode: fail
# llm: none
# expect: 越权承诺
"""一个硬编码的坏草稿：没查订单，还承诺“今天一定退款到账”。闸门必须拦住。"""
import sys
import pathlib

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from ch05.common import GateError, Store, gate_or_raise, known_summary

store = Store()
order = store.order("A1659")
summary = known_summary({"order": order, "policies": []})
draft = "您放心，今天一定退款到账。"

print("[草稿] " + draft)
print("[事实] 这份草稿没有引用订单号，也没有查过政策")
try:
    gate_or_raise(draft, summary, order)
except GateError as e:
    print("[闸门] 拒绝发出：" + str(e), file=sys.stderr)
    sys.exit(1)
print("[闸门] 通过了")
