# 27.4 护栏：T28 被输入护栏拦下，T27 被输出护栏拦下，T06 正常通过。
# mode: run
# expect: T28 输入护栏 triggered=True
# expect: T27 输出护栏 triggered=True
# expect: T06 输入护栏 triggered=False
from agents import (InputGuardrailTripwireTriggered, OutputGuardrailTripwireTriggered,
                    Runner, set_default_openai_api, set_tracing_disabled)
from ch27._lib.guardrails import build_agent
from shop import Store, tickets

set_default_openai_api("chat_completions")
set_tracing_disabled(True)
TICKETS = {t["id"]: t for t in tickets()}


def context_for(store, tid):
    return {"address": store.customer(TICKETS[tid]["customer_id"])["address"]}


# T28：注入文本。输入护栏和模型同一轮启动；本地护栏先命中，模型请求没有发出。
store = Store()
try:
    Runner.run_sync(build_agent(store), TICKETS["T28"]["text"], context=context_for(store, "T28"))
except InputGuardrailTripwireTriggered as e:
    info = e.guardrail_result.output.output_info
    print(f"T28 输入护栏 triggered=True 命中={info['hit']}")

# T27：这里故意用一个没有隐私规则的 agent，看输出护栏能不能兜住别人的地址。
store = Store()
try:
    Runner.run_sync(build_agent(store, leaky=True), TICKETS["T27"]["text"],
                    context=context_for(store, "T27"))
except OutputGuardrailTripwireTriggered as e:
    info = e.guardrail_result.output.output_info
    print(f"T27 输出护栏 triggered=True 泄漏={info['leaked'][0]}")

# T06：正常工单，两道护栏都放行。
store = Store()
result = Runner.run_sync(build_agent(store), TICKETS["T06"]["text"], context=context_for(store, "T06"))
print("T06 输入护栏 triggered="
      f"{result.input_guardrail_results[0].output.tripwire_triggered}，"
      "输出护栏 triggered="
      f"{result.output_guardrail_results[0].output.tripwire_triggered}")
print(f"T06 回答：{result.final_output}")
