# mode: run
# llm: none
# expect: 代理
# expect: vault
# expect: TOKEN
"""凭据边界：sandbox 只有 session token，代理拿 session 去 vault 换上游 token。"""
import json

SESSION = "sess-7c1e"
UPSTREAM = "up-9a3f"          # 上游服务的 token，只在 vault 和代理里出现
VAULT = {SESSION: UPSTREAM}
API_RESULT = {"order_id": "A1680", "status": "已退款"}


def sandbox_read_env() -> dict:
    """sandbox 里能读到的环境变量：只有 PATH。"""
    return {"PATH": "/usr/bin:/bin"}


def sandbox_tool_call() -> dict:
    """模型生成的代码只能看到工具结果，看不到代理手里的 token。"""
    return {"tool": "refund", "result": API_RESULT}


# ANCHOR: proxy
class Proxy:
    def __init__(self):
        self.session = None
        self.upstream = None

    def call(self, session_token: str, tool: str) -> dict:
        self.session = session_token
        self.upstream = VAULT[session_token]      # 代理去 vault 换上游 token
        return {"tool": tool, "upstream": "已认证", "result": API_RESULT}


def main():
    env = sandbox_read_env()
    result = sandbox_tool_call()
    proxy = Proxy()
    out = proxy.call(SESSION, "refund")
    agent_view = json.dumps({"env": env, "tool_result": result}, ensure_ascii=False)
    print("agent 看到:", agent_view)
    print("代理看到: session =", proxy.session, " upstream =", proxy.upstream)
    print("vault 看到:", json.dumps(VAULT, ensure_ascii=False))
    print("代理调用结果:", json.dumps(out, ensure_ascii=False))
    assert "TOKEN" not in agent_view and UPSTREAM not in agent_view
    print("检查: agent 栏没有 TOKEN，也没有上游 token")
# ANCHOR_END: proxy


if __name__ == "__main__":
    main()
