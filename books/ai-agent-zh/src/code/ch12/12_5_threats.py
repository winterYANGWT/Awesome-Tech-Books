# mode: run
# llm: none
# expect: 拒绝
# expect: 8
"""八种输入各跑一遍：路径、NUL、未知工具、超大参数、Content-Type、aud、过期状态。"""
import base64
import hashlib
import hmac
import json

from mcp.server.mcpserver.resources import ResourceSecurity

RESOURCE = "https://books.example/mcp"
ALLOWED_TOOLS = {"get_order", "search_books"}
KEY = b"demo-key-not-for-production"


def b64(raw: bytes) -> str:
    return base64.urlsafe_b64encode(raw).decode().rstrip("=")


def seal(payload: dict) -> str:
    body = b64(json.dumps(payload, separators=(",", ":")).encode())
    return body + "." + hmac.new(KEY, body.encode(), hashlib.sha256).hexdigest()[:16]


def open_state(token: str, now: int) -> tuple[bool, str]:
    body, _, sig = token.partition(".")
    want = hmac.new(KEY, body.encode(), hashlib.sha256).hexdigest()[:16]
    if not hmac.compare_digest(sig, want):
        return False, "签名不匹配"
    payload = json.loads(base64.urlsafe_b64decode(body + "=" * (-len(body) % 4)))
    if payload["exp"] < now:
        return False, "已过期"
    return True, "OK"


def main():
    security = ResourceSecurity()
    # ANCHOR: attempts
    attempts = []

    for label, value in [("../../etc/passwd", "../../etc/passwd"),
                         ("/etc/passwd", "/etc/passwd"),
                         ("B05\\x00../../etc/passwd", "B05\0../../etc/passwd")]:
        bad = security.validate({"value": value})
        if "\0" in value:
            why = "含 NUL 字节"
        elif value.startswith("/"):
            why = "绝对路径"
        else:
            why = "包含 .. 路径段"
        attempts.append((label, bad is None, why))

    name = "drop_all_orders"
    attempts.append((f"未知工具 {name}", name in ALLOWED_TOOLS, "不在工具允许列表"))

    big = "x" * 70_000
    attempts.append((f"超大参数 {len(big)} 字节", len(big.encode()) <= 64 * 1024,
                     "超过 64 KB 上限"))

    ctype = "text/html"
    attempts.append((f"Content-Type {ctype}", ctype.split(";")[0] == "application/json",
                     "必须是 application/json"))

    token = {"aud": "https://other.example/mcp", "exp": 2_000}
    attempts.append(("aud 错误的 token", token["aud"] == RESOURCE,
                     "aud 不是本 server"))

    state = seal({"sub": "C043", "exp": 1_000})
    ok, why = open_state(state, now=2_000)
    attempts.append(("过期的 requestState", ok, why))
    # ANCHOR_END: attempts

    print("| 尝试 | 结果 | 理由 |")
    print("|---|---|---|")
    for label, ok, why in attempts:
        print(f"| {label} | {'通过' if ok else '拒绝'} | {why} |")
    rejected = sum(1 for _, ok, _ in attempts if not ok)
    print(f"共 {len(attempts)} 项：拒绝 {rejected} 项，通过 {len(attempts) - rejected} 项")


if __name__ == "__main__":
    main()
