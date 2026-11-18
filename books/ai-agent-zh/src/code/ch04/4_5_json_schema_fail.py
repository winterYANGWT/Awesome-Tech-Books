"""Chat Completions 的 json_schema 在 DeepSeek 上的最小失败证据：请求直接返回 400。"""
# mode: fail
# expect: json_schema
# expect: 400
import json
import os
import re

from openai import BadRequestError, OpenAI

from shop import tickets

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")

ticket = next(t for t in tickets() if t["id"] == "T06")
response_format = {
    "type": "json_schema",
    "json_schema": {
        "name": "ticket",
        "strict": True,
        "schema": {
            "type": "object",
            "properties": {
                "category": {"type": "string", "enum": ["物流查询", "退货退款", "修改地址", "政策咨询", "投诉建议", "其他", "越界"]},
                "order_id": {"type": "string"},
            },
            "required": ["category", "order_id"],
            "additionalProperties": False,
        },
    },
}
print(f"response_format={response_format['type']}")
try:
    resp = client.chat.completions.create(
        model=MODEL,
        messages=[{"role": "system", "content": "把工单分类，输出 json。"},
                  {"role": "user", "content": ticket["text"]}],
        response_format=response_format,
        max_tokens=600,
    )
except BadRequestError as e:
    # 去掉请求级 ID：它会随每次请求变化，也不是错误本身。
    msg = re.sub(r"\s*\(request_id: [^)]+\)", "", str(e))
    raise RuntimeError(msg) from None
print(json.loads(resp.choices[0].message.content or "{}"))
