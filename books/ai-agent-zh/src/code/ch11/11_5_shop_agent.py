# expect: get_order
# expect: 迅达快递
# expect: 82

"""同步 harness 接异步 Client：事件循环在一个后台线程，harness 在 to_thread 的工作线程里跑。"""

import asyncio
import json
import sys
import threading
from pathlib import Path

from mcp import Client
from mcp.client.stdio import StdioServerParameters

from harness import Tool, run
from shop import tickets
from shop.tools import SUPPORT_PROMPT

CODE = Path(__file__).resolve().parents[1]
SERVER = CODE / "ch11" / "11_2_shop_server" / "server.py"
READ_TIMEOUT = 30
WAIT_TIMEOUT = 35


class MCPToolbox:
    """harness 只依赖 tools / schemas() / call() 三个成员。"""

    def __init__(self, client: Client, loop: asyncio.AbstractEventLoop) -> None:
        self.client = client
        self.loop = loop
        self.tools: dict[str, Tool] = {}

    async def load(self) -> None:
        for remote in (await self.client.list_tools()).tools:
            self.tools[remote.name] = Tool(
                name=remote.name,
                description=remote.description or "",
                parameters=remote.input_schema,
                fn=self._make_fn(remote.name),
            )

    def _make_fn(self, name: str):
        def fn(**kwargs):
            return self.call(name, kwargs)
        return fn

    def schemas(self) -> list[dict]:
        return [t.schema() for t in self.tools.values()]

    def call(self, name: str, arguments) -> str:
        if isinstance(arguments, str):
            try:
                args = json.loads(arguments or "{}")
            except json.JSONDecodeError as e:
                return f"错误：参数不是合法的 JSON（{e.msg}）"
        else:
            args = dict(arguments)

        # ANCHOR: call_short
        future = asyncio.run_coroutine_threadsafe(
            self.client.call_tool(name, args, read_timeout_seconds=READ_TIMEOUT), self.loop)
        try:
            result = future.result(timeout=WAIT_TIMEOUT)
        except TimeoutError:
            future.cancel()
            return "错误：MCP 调用超时；可以重试，或换一种查法"

        text = "\n".join(block.text for block in result.content if getattr(block, "type", None) == "text")
        if result.is_error:
            return "错误：" + (text or "工具执行失败")
        return text or json.dumps(result.structured_content, ensure_ascii=False)
        # ANCHOR_END: call_short


def run_agents(box: MCPToolbox):
    answers = []
    for ticket_id in ("T01", "T06"):
        ticket = next(t for t in tickets() if t["id"] == ticket_id)
        messages = [{"role": "system", "content": SUPPORT_PROMPT},
                    {"role": "user", "content": ticket["text"]}]
        result = run(messages, box, max_turns=8)
        answers.append((ticket_id, result.answer))
    return answers


async def session(answers: list) -> None:
    params = StdioServerParameters(command=sys.executable, args=[str(SERVER)],
                                   env={"PYTHONPATH": str(CODE)}, cwd=str(CODE))
    # ANCHOR: session
    async with Client(params) as client:
        box = MCPToolbox(client, asyncio.get_running_loop())
        await box.load()
        # harness.run 是同步的：放到工作线程，事件循环才能继续处理 call_tool
        answers.extend(await asyncio.to_thread(run_agents, box))
    # ANCHOR_END: session


class LoopThread:
    """一个只跑事件循环的后台线程。"""

    def __init__(self) -> None:
        self.loop = asyncio.new_event_loop()
        self.thread = threading.Thread(target=self.loop.run_forever, daemon=True)
        self.thread.start()

    def run(self, coro):
        return asyncio.run_coroutine_threadsafe(coro, self.loop).result()

    def stop(self) -> None:
        self.loop.call_soon_threadsafe(self.loop.stop)
        self.thread.join()


def main() -> None:
    answers: list = []
    loop_thread = LoopThread()
    try:
        loop_thread.run(session(answers))       # 先停 harness，再退出 Client，最后停循环
    finally:
        loop_thread.stop()
    for ticket_id, answer in answers:
        print(f"[{ticket_id}] 回答：" + " ".join((answer or "").split()))


if __name__ == "__main__":
    main()
