"""书店的数据：每次 `Store()` 都从 `data/*.json` 建一个全新的内存 SQLite，所以示例之间互不影响。

需要跨进程保留状态（第 22 章的崩溃恢复）时用 `Store("shop.db")`：文件不存在就新建，存在就直接打开。
"""
from __future__ import annotations

import datetime as dt
import json
import pathlib
import re
import sqlite3

HERE = pathlib.Path(__file__).resolve().parent
DATA = HERE / "data"
POLICIES = HERE / "policies"
TODAY = dt.date(2026, 11, 17)
REFUND_LIMIT = 500.0          # 单笔退款超过这个数需要主管批准（policies/returns.md）

SCHEMA = """
CREATE TABLE books(id TEXT PRIMARY KEY, title TEXT, author TEXT, publisher TEXT, category TEXT, price REAL);
CREATE TABLE customers(id TEXT PRIMARY KEY, name TEXT, city TEXT, province TEXT, phone_tail TEXT,
                       address TEXT, level TEXT, joined TEXT);
CREATE TABLE orders(id TEXT PRIMARY KEY, customer_id TEXT, created TEXT, discount REAL, shipping_fee REAL,
                    total REAL, address TEXT, courier TEXT, status TEXT, refunded REAL);
CREATE TABLE order_items(order_id TEXT, book_id TEXT, qty INTEGER, price REAL);
CREATE TABLE shipments(order_id TEXT PRIMARY KEY, courier TEXT, shipped TEXT, delivered TEXT, damaged INTEGER,
                       events TEXT);
CREATE TABLE returns(id TEXT PRIMARY KEY, order_id TEXT, book_id TEXT, created TEXT, reason TEXT, amount REAL,
                     status TEXT);
CREATE TABLE reviews(id TEXT PRIMARY KEY, book_id TEXT, customer_id TEXT, order_id TEXT, stars INTEGER,
                     text TEXT, created TEXT);
CREATE TABLE actions(seq INTEGER PRIMARY KEY AUTOINCREMENT, kind TEXT, order_id TEXT, detail TEXT);
"""


class ShopError(Exception):
    """业务规则不允许的操作。消息写给模型看：说清楚为什么不行、可以怎么办。"""


def _rows(cur) -> list[dict]:
    cols = [c[0] for c in cur.description]
    return [dict(zip(cols, r)) for r in cur.fetchall()]


class Store:
    def __init__(self, path: str | pathlib.Path = ":memory:"):
        fresh = str(path) == ":memory:" or not pathlib.Path(path).exists()
        self.db = sqlite3.connect(str(path), check_same_thread=False)
        if fresh:
            self._load()

    # ---- 建库 ---------------------------------------------------------------
    def _load(self) -> None:
        self.db.executescript(SCHEMA)
        load = lambda name: json.loads((DATA / f"{name}.json").read_text(encoding="utf-8"))  # noqa: E731
        ins = lambda table, row: self.db.execute(  # noqa: E731
            f"INSERT INTO {table}({','.join(row)}) VALUES({','.join('?' * len(row))})", list(row.values()))
        for b in load("books"):
            ins("books", b)
        for c in load("customers"):
            ins("customers", c)
        for o in load("orders"):
            items = o.pop("items")
            ins("orders", o)
            for it in items:
                ins("order_items", {"order_id": o["id"], **it})
        for s in load("shipments"):
            s["events"] = json.dumps(s["events"], ensure_ascii=False)
            s["damaged"] = int(s["damaged"])
            ins("shipments", s)
        for r in load("returns"):
            ins("returns", r)
        for v in load("reviews"):
            ins("reviews", v)
        self.db.commit()

    def query(self, sql: str, *args) -> list[dict]:
        return _rows(self.db.execute(sql, args))

    # ---- 查询 ---------------------------------------------------------------
    def search_books(self, text: str, limit: int = 5) -> list[dict]:
        words = [w for w in re.split(r"[\s，,、]+", text.strip()) if w]
        if not words:
            return []
        where = " AND ".join("(title LIKE ? OR author LIKE ? OR category LIKE ? OR publisher LIKE ?)" for _ in words)
        args = [a for w in words for a in (f"%{w}%",) * 4]
        return self.query(f"SELECT * FROM books WHERE {where} ORDER BY id LIMIT ?", *args, limit)

    def book(self, book_id: str) -> dict | None:
        rows = self.query("SELECT * FROM books WHERE id = ?", book_id)
        return rows[0] if rows else None

    def customer(self, customer_id: str) -> dict | None:
        rows = self.query("SELECT * FROM customers WHERE id = ?", customer_id)
        return rows[0] if rows else None

    def find_customers(self, name: str | None = None, phone_tail: str | None = None) -> list[dict]:
        sql, args = "SELECT * FROM customers WHERE 1", []
        if name:
            sql, args = sql + " AND name = ?", args + [name]
        if phone_tail:
            sql, args = sql + " AND phone_tail = ?", args + [phone_tail]
        return self.query(sql + " ORDER BY id", *args)

    def orders_of(self, customer_id: str, limit: int = 10) -> list[dict]:
        return self.query("SELECT id, created, total, status, refunded FROM orders WHERE customer_id = ? "
                          "ORDER BY created DESC, id DESC LIMIT ?", customer_id, limit)

    def order(self, order_id: str) -> dict | None:
        rows = self.query("SELECT * FROM orders WHERE id = ?", order_id)
        if not rows:
            return None
        o = rows[0]
        o["items"] = self.query("SELECT i.book_id, b.title, b.category, i.qty, i.price FROM order_items i "
                                "JOIN books b ON b.id = i.book_id WHERE i.order_id = ? ORDER BY b.id", order_id)
        o["shipment"] = self.shipment(order_id)
        return o

    def shipment(self, order_id: str) -> dict | None:
        rows = self.query("SELECT * FROM shipments WHERE order_id = ?", order_id)
        if not rows:
            return None
        s = rows[0]
        s["events"] = json.loads(s["events"])
        s["damaged"] = bool(s["damaged"])
        return s

    def reviews_of(self, book_id: str, limit: int = 5) -> list[dict]:
        return self.query("SELECT id, stars, text, created FROM reviews WHERE book_id = ? "
                          "ORDER BY created DESC, id DESC LIMIT ?", book_id, limit)

    def returns_of(self, order_id: str) -> list[dict]:
        return self.query("SELECT * FROM returns WHERE order_id = ? ORDER BY id", order_id)

    # ---- 政策文档 -------------------------------------------------------------
    @staticmethod
    def policy_names() -> list[str]:
        return sorted(p.stem for p in POLICIES.glob("*.md"))

    @staticmethod
    def policy(name: str) -> str:
        p = POLICIES / f"{name}.md"
        if not p.exists():
            raise ShopError(f"没有名为 {name} 的政策文档，可选：{', '.join(Store.policy_names())}")
        return p.read_text(encoding="utf-8")

    @staticmethod
    def policy_sections() -> list[dict]:
        out = []
        for name in Store.policy_names():
            text = Store.policy(name)
            doc_title = text.splitlines()[0].lstrip("# ").strip()
            for part in re.split(r"(?m)^## ", text)[1:]:
                head, _, body = part.partition("\n")
                out.append({"doc": name, "title": f"{doc_title} / {head.strip()}", "text": body.strip()})
            if "\n## " not in text:
                out.append({"doc": name, "title": doc_title, "text": text.partition("\n")[2].strip()})
        return out

    @staticmethod
    def search_policies(query: str, limit: int = 3) -> list[dict]:
        """按查询里的两字词片在各节里出现的次数打分，返回得分最高的几节。"""
        q = re.sub(r"\s+", "", query)
        grams = {q[i:i + 2] for i in range(len(q) - 1)} or {q}
        scored = []
        for s in Store.policy_sections():
            hay = s["title"] + s["text"]
            score = sum(hay.count(g) for g in grams)
            if score:
                scored.append((score, s))
        scored.sort(key=lambda x: -x[0])
        return [s for _, s in scored[:limit]]

    # ---- 会改数据的操作 -------------------------------------------------------
    def _log(self, kind: str, order_id: str, detail: dict) -> None:
        self.db.execute("INSERT INTO actions(kind, order_id, detail) VALUES(?, ?, ?)",
                        (kind, order_id, json.dumps(detail, ensure_ascii=False)))
        self.db.commit()

    def refund(self, order_id: str, amount: float, reason: str, approved_by: str | None = None) -> dict:
        o = self.order(order_id)
        if o is None:
            raise ShopError(f"订单 {order_id} 不存在")
        if o["status"] in ("待发货", "已取消"):
            raise ShopError(f"订单 {order_id} 状态是“{o['status']}”，没有可退的款项")
        amount = round(float(amount), 2)
        if amount <= 0:
            raise ShopError("退款金额必须大于 0")
        left = round(o["total"] - o["refunded"], 2)
        if amount > left + 1e-9:
            raise ShopError(f"订单 {order_id} 实付 {o['total']:.2f} 元，已退 {o['refunded']:.2f} 元，最多还能退 {left:.2f} 元")
        if amount > REFUND_LIMIT and not approved_by:
            raise ShopError(f"单笔退款超过 {REFUND_LIMIT:.0f} 元需要主管批准，请改为提交主管审批，不要直接退款")
        refunded = round(o["refunded"] + amount, 2)
        status = "已退款" if refunded >= o["total"] - 0.01 else "部分退款"
        self.db.execute("UPDATE orders SET refunded = ?, status = ? WHERE id = ?", (refunded, status, order_id))
        self._log("refund", order_id, {"amount": amount, "reason": reason, "approved_by": approved_by})
        return {"order_id": order_id, "amount": amount, "refunded_total": refunded, "status": status}

    def update_address(self, order_id: str, address: str) -> dict:
        o = self.order(order_id)
        if o is None:
            raise ShopError(f"订单 {order_id} 不存在")
        if o["status"] != "待发货":
            raise ShopError(f"订单 {order_id} 状态是“{o['status']}”，只有待发货的订单能改地址")
        self.db.execute("UPDATE orders SET address = ? WHERE id = ?", (address, order_id))
        self._log("update_address", order_id, {"old": o["address"], "new": address})
        return {"order_id": order_id, "address": address}

    def escalate(self, order_id: str | None, note: str) -> dict:
        self._log("escalate", order_id or "", {"note": note})
        return {"ticket": f"E{self.query('SELECT COUNT(*) AS n FROM actions')[0]['n']:03d}", "status": "已提交主管"}

    def actions(self) -> list[dict]:
        rows = self.query("SELECT kind, order_id, detail FROM actions ORDER BY seq")
        for r in rows:
            r["detail"] = json.loads(r["detail"])
        return rows

    # ---- 只读 SQL（第 14、18 章）-----------------------------------------------
    def readonly_sql(self, sql: str, max_rows: int = 50) -> list[dict]:
        def deny_writes(action, *_):
            ok = action in (sqlite3.SQLITE_SELECT, sqlite3.SQLITE_READ, sqlite3.SQLITE_FUNCTION)
            return sqlite3.SQLITE_OK if ok else sqlite3.SQLITE_DENY
        self.db.set_authorizer(deny_writes)
        try:
            cur = self.db.execute(sql)
            cols = [c[0] for c in cur.description]
            return [dict(zip(cols, r)) for r in cur.fetchmany(max_rows)]
        except sqlite3.DatabaseError as e:
            raise ShopError(f"SQL 执行失败：{e}") from None
        finally:
            self.db.set_authorizer(None)
