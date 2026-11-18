"""18.3 录到的三份子 agent 结果，18.4 和 18.6 直接读它。

数值来自 `shop` 的固定数据；每条的 source 对应 18.3 输出里的表名/字段或文档小节。
"""

FINDINGS = {
    "sql": {
        "status": "ok",
        "gaps": ["Q4 只到 2026-11-16，12 月的数据还没有",
                 "查询 3 没有按单本书拆分，B07/B12/B19 各自的缺页/装订笔数还缺"],
        "findings": [
            {"claim": "2026-Q3 退货率 18.3%（38/208），Q4 回落到 8.2%（8/98）。",
             "evidence": "Q1 5.6%（10/178）、Q2 5.4%（10/185）、Q3 18.3%（38/208）、Q4 8.2%（8/98），已排除取消订单；Q3 约为 Q1/Q2 的 3.4 倍。",
             "source": "returns / orders，按 orders.created 分季度"},
            {"claim": "Q3 缺页 15 笔、装订脱落 5 笔，合计 20 笔。",
             "evidence": "Q3 的 returns.reason 计数：缺页 15、装订脱落 5、运输破损 9、买错版本 7；缺页加装订占 Q3 38 笔退货的 52.6%。",
             "source": "returns.reason"},
            {"claim": "青川出版社 6 月那批 B07、B12、B19 贡献了这 20 笔缺页和装订脱落。",
             "evidence": "Q3 这三本书的缺页+装订脱落 20 笔；青川出版社 Q3 退货共 22 笔。",
             "source": "returns JOIN books，publisher='青川出版社'"},
            {"claim": "迅达快递的破损率是 24.6%，顺路是 1.0%。",
             "evidence": "迅达 16/65=24.6%，顺路 6/601=1.0%；运输破损退货 Q3 9 笔、Q4 8 笔。",
             "source": "shipments.damaged JOIN orders.courier"},
            {"claim": "7 月暑期促销带来 7 笔教辅买错版本。",
             "evidence": "Q3 买错版本 7 笔，全部是教辅类；Q2 只有 2 笔买错了。",
             "source": "returns.reason JOIN books.category"},
            {"claim": "Q4 回落后仍有 8 笔运输破损，不能说快递问题已解决。",
             "evidence": "Q4 运输破损 8 笔；Q4 退货率 8.2%（8/98），数据截止 2026-11-16。",
             "source": "returns.reason / orders.created"},
        ],
    },
    "reviews": {
        "status": "ok",
        "gaps": ["B12 只有 2 条评论，且都不是差评；评论只能佐证，不能替代退货记录"],
        "findings": [
            {"claim": "B07 有 11 条评论，其中 1 条 1 星说缺页、1 条 2 星说快递把书脊弄裂。",
             "evidence": "1 星：“缺页！这个批次是不是有问题？”（2026-07-10）；2 星：“快递太粗暴，书脊都裂了。”（2026-10-22）。",
             "source": "reviews，book_id='B07'"},
            {"claim": "B19 有 9 条评论，1 条 1 星说第 120 到 150 页整段缺失。",
             "evidence": "1 星：“第 120 页到 150 页整个缺了，退了。”（2026-09-20）。",
             "source": "reviews，book_id='B19'"},
            {"claim": "B12 只有 2 条评论，都不是差评，样本不足以支撑结论。",
             "evidence": "2 条评论是 3 星“内容偏浅”和 4 星“物流很快，包装完好”，没有 Q3 的评论。",
             "source": "reviews / returns，book_id='B12'"},
        ],
    },
    "policy": {
        "status": "ok",
        "gaps": ["政策只能证明条款变了，不能证明它是 Q3 退货率上升的主因"],
        "findings": [
            {"claim": "退货期限从 2026-07-01 起由 7 天改为 15 天。",
             "evidence": "returns.md 修订记录：2026-07-01 退货期限从 7 天改为 15 天。",
             "source": "returns.md / 退货期限、修订记录"},
            {"claim": "质量问题（缺页、装订脱落、运输破损）签收后 60 天内退全款。",
             "evidence": "returns.md 质量问题一节：质量问题退全款，不受 15 天限制，60 天内可申请。",
             "source": "returns.md / 质量问题"},
            {"claim": "2026-08-01 起广东、广西、海南的订单改用迅达快递。",
             "evidence": "shipping.md 快递公司一节：2026-08-01 起三地改由迅达配送。",
             "source": "shipping.md / 快递公司"},
        ],
    },
}


def all_findings() -> list[dict]:
    return [f for part in FINDINGS.values() for f in part["findings"]]
