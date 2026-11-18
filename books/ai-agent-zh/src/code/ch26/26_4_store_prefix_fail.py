# 26.4 反例：namespace 第一段是类别时，search(("profile",)) 会跨顾客。
# mode: fail
# llm: none
# expect: C083

from langgraph.store.memory import InMemoryStore

store = InMemoryStore()
# 有人按“类别/顾客”放，而不是“顾客/类别”：第一段 profile 是共享的。
store.put(("profile", "C082"), "invoice", {"title": "天津远山文化"})
store.put(("profile", "C083"), "invoice", {"title": "苏州远山文化"})

hits = store.search(("profile",))
ids = sorted({item.namespace[1] for item in hits})
print("search(('profile',)) 返回的顾客:", ", ".join(ids))
assert ids == ["C082"], f"只应看到 C082，实际看到了 {', '.join(ids)}"
