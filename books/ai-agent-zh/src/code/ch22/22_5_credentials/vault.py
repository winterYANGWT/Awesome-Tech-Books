"""假凭据库：token 只在这个对象里，沙箱进程拿不到。"""
from __future__ import annotations


class Vault:
    def __init__(self) -> None:
        self._tokens = {"mcp:orders": "tok_demo_mcp_orders_not_a_real_secret"}

    def get(self, audience: str) -> str | None:
        return self._tokens.get(audience)
