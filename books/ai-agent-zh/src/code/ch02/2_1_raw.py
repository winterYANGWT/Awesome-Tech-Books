"""2.1 读录制文件，看这次请求真正发出去的字段，没有隐藏字段。"""
# llm: none
# expect: role=system
from harness import cassette

call = cassette.load("ch02/2_1_first_call.cassette.jsonl")[0]
request, response = call.request, call.response

print("请求顶层键：" + ", ".join(sorted(request)))
print(f"消息 {len(request['messages'])} 条：")
for message in request["messages"]:
    content = message.get("content") or ""
    print(f"  role={message['role']} content={len(content)} 字")

print("响应 choices[0] 键：" + ", ".join(sorted(response["choices"][0])))
print("响应 message 键：" + ", ".join(sorted(response["choices"][0]["message"])))
print("响应 usage 键：" + ", ".join(sorted(response["usage"])))
print("有没有 tools 字段：" + ("有" if "tools" in request else "没有"))
