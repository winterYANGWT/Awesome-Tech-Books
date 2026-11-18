"""同一个 json_schema，换到 Responses 接口的 text.format 就能用。"""
# expect: json_schema
# expect: 退货退款
import json
import os

from openai import OpenAI

from shop import tickets

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")

ticket = next(t for t in tickets() if t["id"] == "T06")
schema = {
    "type": "object",
    "properties": {
        "category": {"type": "string", "enum": ["物流查询", "退货退款", "修改地址", "政策咨询", "投诉建议", "其他", "越界"]},
        "order_id": {"type": "string"},
    },
    "required": ["category", "order_id"],
    "additionalProperties": False,
}
fmt = {"type": "json_schema", "name": "ticket", "strict": True, "schema": schema}
print("Responses 接口：text.format=json_schema")
resp = client.responses.create(
    model=MODEL,
    input=[{"role": "system", "content": "把工单分类，输出 json。"},
           {"role": "user", "content": ticket["text"]}],
    text={"format": fmt},
)
obj = json.loads(resp.output_text)
print(f"解析出的对象：{json.dumps(obj, ensure_ascii=False)}")
