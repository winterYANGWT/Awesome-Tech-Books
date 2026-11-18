"""2.1 反例：`messages=[]`，服务端在进模型之前就拒绝。"""
# mode: fail
# expect: messages
import json
import os
import re

from openai import BadRequestError, OpenAI

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")

try:
    client.chat.completions.create(model=MODEL, messages=[])
except BadRequestError as exc:
    body = exc.body
    if isinstance(body, str):
        try:
            body = json.loads(body)
        except ValueError:
            body = {}
    error = body.get("error", body) if isinstance(body, dict) else {}
    message = error.get("message") or str(exc)
    message = re.sub(r"\s*\(request_id: [^)]+\)", "", message)   # 去掉随机的 request id
    print(f"status_code={exc.status_code}")
    print(f"error.type={exc.type}")
    print(f"error.message={message}")
    raise SystemExit(message) from None
