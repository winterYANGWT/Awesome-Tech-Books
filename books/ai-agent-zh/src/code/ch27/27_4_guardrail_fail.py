# 27.4 反例：不捕获 T28 的 tripwire，进程以 InputGuardrailTripwireTriggered 退出。
# mode: fail
# expect: InputGuardrailTripwireTriggered
from agents import Runner, set_default_openai_api, set_tracing_disabled
from ch27._lib.guardrails import build_agent
from shop import Store, tickets

set_default_openai_api("chat_completions")
set_tracing_disabled(True)

store = Store()
t = next(t for t in tickets() if t["id"] == "T28")
context = {"address": store.customer(t["customer_id"])["address"]}
Runner.run_sync(build_agent(store), t["text"], context=context)
