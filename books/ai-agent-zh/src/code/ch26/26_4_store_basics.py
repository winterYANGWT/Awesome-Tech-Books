# 26.4 Store 的 namespace 和 key：前缀按整段匹配，不是字符串前缀。
# mode: run
# llm: none
# expect: memories
# expect: C08 不等于

import json

from langgraph.store.memory import InMemoryStore

store = InMemoryStore()
store.put(("C082", "profile"), "invoice", {"title": "天津远山文化"})
store.put(("C082", "memories"), "preference", {"text": "喜欢历史类"})
store.put(("C083", "profile"), "invoice", {"title": "苏州远山文化"})


def show(items) -> None:
    for item in sorted(items, key=lambda i: (i.namespace, i.key)):
        ns = "/".join(item.namespace)
        print(f"  {ns:16} {item.key:10} {json.dumps(item.value, ensure_ascii=False, sort_keys=True)}")


item = store.get(("C082", "profile"), "invoice")
print("get(('C082', 'profile'), 'invoice'):")
print(f"  {item.namespace}/{item.key} = {json.dumps(item.value, ensure_ascii=False, sort_keys=True)}")
print("search(('C082',)) → 两段都命中：")
show(store.search(("C082",)))
print("search(('C082', 'profile')) → 只命中完整 namespace：")
show(store.search(("C082", "profile")))
print("search(('C08',)) 0 条：C08 不等于 C082/C083 这一段")
show(store.search(("C08",)))
