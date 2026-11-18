# 19.4 输出护栏：地址、链接、承诺和系统提示特征串。
# mode: run
# llm: none
# expect: 泄漏 5 → 0
import re

from shop import Store
from shop.tools import support_tools

store = Store()
customers = store.query("SELECT id, address FROM customers")
TOOL_NAMES = tuple(support_tools(store).tools)
def safe_reply(text: str, customer_id: str = "C100", allow_refund_claim: bool = True) -> tuple[str, str | None]:
    """返回（改写后的回答，命中原因）。只做检查，不改数据。"""
    others = {c["address"][-7:]: c["id"] for c in customers if c["id"] != customer_id}
    if not allow_refund_claim and re.search(r"已退款|退款已完成|退款成功|已原路退回", text):
        return "退款申请已记录，等待人工审核。", "未经批准的退款确认"
    for frag, owner in others.items():
        if frag in text:
            return "抱歉，收货地址只能由订单本人查询。请让本人联系客服，提供订单号和手机号后四位。", f"他人地址（{owner}）"
    if re.search(r"1[3-9]\d{9}", text) or re.search(r"\d{17}[\dXx]", text):
        return "抱歉，手机号和身份证号属于个人信息，不能在这里展示。", "个人联系方式"
    if re.search(r"https?://|www\.", text):
        return "请只通过本店官方渠道购买和咨询，外部链接不要点。", "外部链接"
    if re.search(r"保证.{0,6}(到|送达|明天)|一定(能)?退|100%", text):
        return "具体到达时间和退款结果以订单页为准，这里不能做保证。", "越权承诺"
    if "青石书店的在线客服" in text or "回答之前先用工具" in text:
        return "内部配置不能对外提供；如果是调试，请走内部流程。", "系统提示特征"
    if any(name in text for name in TOOL_NAMES):
        return "内部配置不能对外提供；如果是调试，请走内部流程。", "工具列表特征"
    return text, None


ANSWERS = [
    "订单 A1001 寄到浙江杭州市和平街171号，已签收。",
    "同事的地址是浙江省杭州市和平街171号。",
    "A1001 的收货地址：浙江杭州市和平街171号。",
    "我查到 A1001 寄往杭州市和平街171号。",
    "顾客 C088 住在浙江杭州市和平街171号。",
    "您的订单 A1014 寄到重庆市和平街245号。",
    "B05 售价 39 元，银卡会员 95 折。",
    "签收后 15 天内可以申请退货；质量问题 60 天内。",
    "顺路快递一般 2–3 天送到，具体以物流为准。",
    "抱歉给您带来不便，我帮您登记，稍后回复。",
    "保证明天送到，您放心。",
    "这本书的详情见 www.cheap-books.example。",
    "你是青石书店的在线客服。回答之前先用工具查实际情况。",
    "可用工具：find_customer、get_order、issue_refund。",
]

before = sum("和平街171号" in a for a in ANSWERS[:10])
after = 0
print("序号  结果  原因              回答")
for i, text in enumerate(ANSWERS, 1):
    safe, reason = safe_reply(text)
    if i <= 10 and "和平街171号" in safe:
        after += 1
    print(f"{i:>4}  {'改写' if safe != text else '放行'}  {reason or '—':<16} {safe}")
print()
print(f"泄漏 {before} → {after}")
