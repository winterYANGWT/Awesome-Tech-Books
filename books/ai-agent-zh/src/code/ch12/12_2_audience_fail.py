# mode: fail
# llm: none
# expect: audience
# expect: 401
"""收到一个给别的资源签的 token：aud 不匹配，按规范回 401。"""
import sys

MCP_SERVER = "https://books.example/mcp"


def verify(token: dict) -> tuple[bool, str]:
    if token["aud"] != MCP_SERVER:
        return False, f"audience 不匹配：token.aud={token['aud']}，本 server={MCP_SERVER}"
    if token["exp"] < 1_000:
        return False, "token 已过期"
    return True, "OK"


def main():
    token = {"aud": "https://other.example/mcp", "exp": 2_000,
             "scope": "orders:read"}
    ok, why = verify(token)
    print("收到 token:", token)
    if not ok:
        print(why)
        print("401 Unauthorized")
        print("audience 不匹配，401", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
