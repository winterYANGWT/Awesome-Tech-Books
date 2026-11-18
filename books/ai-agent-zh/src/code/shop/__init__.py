"""贯穿全书的示例：一家虚构的网上书店“青石书店”。

    from shop import Store, TODAY
    store = Store()                      # 全新的内存数据库，示例之间互不影响
    store.order("A1583")

数据由 `scripts/make_shop_data.py` 生成（固定随机种子）；政策文档在 `policies/`。
客服 agent 的工具在 `shop.tools`，评测用的 30 个工单在 `shop.tickets()`。
"""
from __future__ import annotations

import json

from .store import DATA, POLICIES, REFUND_LIMIT, TODAY, ShopError, Store

__all__ = ["Store", "ShopError", "TODAY", "REFUND_LIMIT", "POLICIES", "tickets"]


def tickets() -> list[dict]:
    """30 个客服工单：顾客、类别、原话，以及正确处理后应达到的终态（第 5、21 章）。"""
    return json.loads((DATA / "tickets.json").read_text(encoding="utf-8"))
