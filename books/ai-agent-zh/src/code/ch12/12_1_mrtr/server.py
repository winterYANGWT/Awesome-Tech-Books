"""退款 server：单笔超过 500 元时，用 MRTR 请主管批准。"""
from typing import Annotated

from pydantic import BaseModel, Field
from mcp.server.mcpserver import Context, Elicit, MCPServer, Resolve
from shop import Store

mcp = MCPServer("bookshop-refund")
store = Store()


# ANCHOR: resolver
class Approval(BaseModel):
    approved: bool = Field(description="主管是否批准这笔退款")
    note: str = Field(default="", description="主管编号，例如 M01")


def approval(order_id: str, amount: float) -> Elicit[Approval] | Approval:
    if amount <= 500:
        return Approval(approved=True, note="rule")
    return Elicit(f"批准 {order_id} 退款 {amount:.0f} 元？", Approval)


@mcp.tool()
def refund(order_id: str, amount: float,
           ok: Annotated[Approval, Resolve(approval)]) -> dict:
    """给订单退款，钱原路退回。单笔超过 500 元时先请主管批准。"""
    if not ok.approved:
        raise ValueError("主管没有批准这笔退款")
    return store.refund(order_id, amount, "装订脱落", approved_by=ok.note)
# ANCHOR_END: resolver


# ANCHOR: old
@mcp.tool()
async def refund_old(order_id: str, amount: float, ctx: Context) -> dict:
    """旧写法：在连接上主动发 elicitation/create。2026-07-28 的 stdio 没有回程通道。"""
    r = await ctx.elicit(f"批准 {order_id} 退款 {amount:.0f} 元？", Approval)
    return {"action": r.action}
# ANCHOR_END: old


if __name__ == "__main__":
    mcp.run()
