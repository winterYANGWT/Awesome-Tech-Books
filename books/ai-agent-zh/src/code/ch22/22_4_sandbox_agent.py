# 22.4 agent 用 run_python 算 76 + 6，再用 get_order 核对，最后回答 82。
# mode: run
# expect: run_python
# expect: 82
"""代码执行也是一个工具：名字、输入、一段字符串输出，harness 不知道后面是容器还是子进程。"""
from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Annotated

from harness import run, tool
from shop import Store
from shop.tools import SUPPORT_PROMPT, support_tools


class Sandbox:
    """和 22_4_sandbox_limits.py 里同一段 execute()；示例目录不能互相 import。"""

    def __init__(self, workdir: Path):
        self.workdir = workdir

    def execute(self, name: str, input: str, timeout: float = 5.0) -> str:
        if name != "run_python":
            return f"error：没有 {name} 这个执行环境"
        if ".." in input:
            return "denied：代码里出现 .."
        try:
            cp = subprocess.run([sys.executable, "-I", "-c", input], cwd=self.workdir,
                                timeout=timeout, env={}, capture_output=True, text=True)
        except subprocess.TimeoutExpired:
            return f"timeout：超过 {timeout:g}s"
        out = (cp.stdout or "").rstrip("\n")
        if len(out) > 200:
            return f"truncated：200/{len(out)} 字符"
        if cp.returncode != 0:
            return f"error：exit={cp.returncode}"
        return out


# ANCHOR: main
def main() -> None:
    store = Store()
    with tempfile.TemporaryDirectory() as tmp:
        sandbox = Sandbox(Path(tmp))

        @tool
        def run_python(code: Annotated[str, "要执行的 Python 代码，只能打印结果，不要读写文件"]) -> str:
            """在隔离的 Python 子进程里运行一小段代码，返回 stdout；超时和过长的输出会被截断。"""
            return sandbox.execute("run_python", code)

        box = support_tools(store, include=("get_order",))
        box.add(run_python)
        messages = [
            {"role": "system", "content": SUPPORT_PROMPT + "\n算术必须用 run_python，不要心算。"},
            {"role": "user", "content": "订单 A1659 缺页，要按实付全额退款。书价 76 元，运费 6 元；"
                                        "先用 get_order 核对，再用 run_python 算 76 + 6；最后只回答应退金额和计算式，"
                                        "不要讨论审批或退款流程。"},
        ]
        result = run(messages, box, max_turns=4)
        print(f"\n最终回答：{result.answer}")
# ANCHOR_END: main


if __name__ == "__main__":
    main()
