---
name: refund-handling
description: 处理订单退款、计算退货金额、判断是否需要主管批准；用户提到退款、退货、退钱、缺页、破损、金额、500 元、拆封或部分退货时使用；用户说“退一下”“能退吗”时也使用。
license: MIT
compatibility: 需要 Python 3.12 和本书的 shop 工具（get_order、issue_refund、escalate_to_supervisor）。
metadata:
  version: "1.2"
  owner: 客服组
allowed-tools: get_order issue_refund escalate_to_supervisor
---
# 退款处理

## 何时用

顾客要求退款、退货、换货折现，或询问“能退多少”“要不要主管批准”时使用。
只处理订单退款；物流查询、发票、会员问题不要用这个 Skill。

## 步骤

1. 用 `get_order` 查订单：核对状态、签收日期、书目、实付、已退金额和运费。
   订单号不存在时让顾客核对订单号，不要猜，也不要用别的订单代替。
2. 判断原因：
   - 缺页、错装、印刷模糊、装订脱落、运输破损：质量问题。
   - 不想要、买错、重复购买：非质量问题。
3. 在 `refund-handling/` 目录下运行脚本，不要自己心算：

   ```bash
   python scripts/refund.py --order A1659 --reason quality
   ```

   非质量问题用 `--reason changed_mind`；顾客说教辅已经拆开时加 `--opened`。
   脚本 stdout 是 JSON，读 `amount`、`needs_approval`、`refundable`、`basis`；stderr 是诊断，不要当结果。
4. `needs_approval` 为 true（金额超过 500 元）：调用 `escalate_to_supervisor`，把订单号、金额、原因写进 note；不要调用 `issue_refund`。
5. 其余可退情况调用 `issue_refund(order_id, amount, reason)`；金额用脚本的 `amount`，保留两位小数。
6. 回复顾客：先给结论，再给金额和到账时间（1–3 个工作日），最后给下一步。

## 坑

- 教辅、考试用书拆封后不退，除非是质量问题；非质量问题直接说明，不要运行退款。
- 部分退货只退所退那几本的书款，原订单的运费不退。
- 非质量问题不退运费；质量问题退全款（书款加运费），签收后 60 天内都算。
- 一张订单累计退款不能超过实付；先看 `get_order` 返回的 `refunded`。
- 金额 100 元以下的质量问题不用要求顾客寄回。
- 不要承诺“马上到账”；到账时间是 1–3 个工作日。

## 输出格式

给顾客的回复按这个顺序：

1. 一句话结论（能退 / 不能退 / 已提交主管）。
2. 金额和计算依据（书款、运费、折扣）。
3. 到账时间或下一步。

需要长回复时读 `assets/refund_reply.md` 的模板；需要引用政策原文时读 `references/returns.md` 的对应小节。

## 参考文件什么时候读

- 顾客追问政策依据，或要引用条款：读 `references/returns.md`，只读相关的一节。
- 要写正式回复、道歉或长邮件：读 `assets/refund_reply.md`。
- 只是算金额、回答“能退多少”：不用读这两个文件，脚本输出就够了。
