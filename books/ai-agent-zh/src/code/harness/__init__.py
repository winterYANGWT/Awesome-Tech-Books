"""第 3 章写出来的最小 harness：工具箱加 agent 循环。后面各章直接 `from harness import ...`。

    from harness import tool, Toolbox, run

    @tool
    def add(a: int, b: int) -> int:
        "两个整数相加"
        return a + b

    result = run([{"role": "user", "content": "3 加 4 是多少？"}], Toolbox([add]))
    print(result.answer)

`run` 每一步打印一行（关掉用 `verbose=False`）：

    第 1 轮 → add(a=3, b=4)  ← 7
"""
from __future__ import annotations

import inspect
import json
import os
import re
import typing
from dataclasses import dataclass, field
from typing import Any, Callable

__all__ = ["MODEL", "Tool", "ToolError", "tool", "Toolbox", "Step", "Result", "run", "short", "fmt_call"]

MODEL = os.environ.get("MODEL", "deepseek-flash")
_client = None


def client():
    """共用一个 OpenAI 客户端（读 OPENAI_BASE_URL、OPENAI_API_KEY）。"""
    global _client
    if _client is None:
        from openai import OpenAI
        _client = OpenAI()
    return _client


class ToolError(Exception):
    """工具主动报告的错误：消息会原样交给模型，所以要写清楚“错在哪、该怎么改”。"""


@dataclass
class Tool:
    name: str
    description: str
    parameters: dict
    fn: Callable[..., Any]
    risky: bool = False            # 有副作用、不可撤销（退款、发消息）：第 19、20 章用它做审批

    def schema(self) -> dict:
        return {"type": "function",
                "function": {"name": self.name, "description": self.description, "parameters": self.parameters}}

    def __call__(self, *args, **kwargs):
        return self.fn(*args, **kwargs)


_JSON_TYPES = {str: "string", int: "integer", float: "number", bool: "boolean"}


def _param_schema(annotation) -> dict:
    desc = None
    if typing.get_origin(annotation) is typing.Annotated:
        annotation, *meta = typing.get_args(annotation)
        desc = next((m for m in meta if isinstance(m, str)), None)
    origin = typing.get_origin(annotation)
    if origin is typing.Literal:
        values = list(typing.get_args(annotation))
        schema = {"type": _JSON_TYPES[type(values[0])], "enum": values}
    elif origin in (typing.Union, getattr(__import__("types"), "UnionType", None)):
        inner = [a for a in typing.get_args(annotation) if a is not type(None)]
        schema = _param_schema(inner[0])
    elif origin is list:
        (item,) = typing.get_args(annotation) or (str,)
        schema = {"type": "array", "items": _param_schema(item)}
    else:
        schema = {"type": _JSON_TYPES.get(annotation, "string")}
    if desc:
        schema["description"] = desc
    return schema


def tool(fn=None, *, name: str | None = None, risky: bool = False):
    """把一个带类型注解的函数变成 Tool。

    描述取文档字符串；参数的说明写成 `Annotated[str, "说明"]`；有默认值的参数不是必填。
    """
    def wrap(f):
        hints = typing.get_type_hints(f, include_extras=True)
        props, required = {}, []
        for pname, p in inspect.signature(f).parameters.items():
            props[pname] = _param_schema(hints.get(pname, str))
            if p.default is inspect.Parameter.empty:
                required.append(pname)
        params = {"type": "object", "properties": props, "required": required}
        return Tool(name or f.__name__, inspect.cleandoc(f.__doc__ or ""), params, f, risky)
    return wrap(fn) if fn else wrap


class Toolbox:
    """按名字找到工具并执行。任何错误都变成一段给模型看的文字，而不是让程序崩掉。"""

    def __init__(self, tools=()):
        self.tools: dict[str, Tool] = {}
        for t in tools:
            self.add(t)

    def add(self, t: Tool) -> None:
        self.tools[t.name] = t

    def schemas(self) -> list[dict]:
        return [t.schema() for t in self.tools.values()]

    def __contains__(self, name: str) -> bool:
        return name in self.tools

    def call(self, name: str, arguments: str | dict) -> str:
        t = self.tools.get(name)
        if t is None:
            return f"错误：没有名为 {name} 的工具。可用的工具：{', '.join(self.tools)}"
        try:
            args = json.loads(arguments or "{}") if isinstance(arguments, str) else dict(arguments)
        except json.JSONDecodeError as e:
            return f"错误：参数不是合法的 JSON（{e.msg}）"
        try:
            out = t.fn(**args)
        except ToolError as e:
            return f"错误：{e}"
        except TypeError as e:
            # Python 的报错带着内部函数名（例如 support_tools.<locals>.get_order()），换成模型认识的工具名
            detail = re.sub(r"^[\w.<>]+\(\)", f"{name}()", str(e))
            return f"错误：参数不对（{detail}）。参数格式：{json.dumps(t.parameters, ensure_ascii=False)}"
        except Exception as e:  # noqa: BLE001 - 工具内部的 bug 也交给模型，并保留类型名方便排查
            return f"错误：工具执行失败（{type(e).__name__}: {e}）"
        return out if isinstance(out, str) else json.dumps(out, ensure_ascii=False)


@dataclass
class Step:
    turn: int
    name: str
    arguments: dict
    result: str


@dataclass
class Result:
    answer: str | None
    messages: list
    steps: list[Step] = field(default_factory=list)
    turns: int = 0
    stop: str = "answer"            # answer：模型不再调用工具；max_turns：轮数用完
    usage: dict = field(default_factory=dict)


def short(text: str, n: int = 40) -> str:
    text = " ".join(str(text).split())
    return text if len(text) <= n else text[: n - 1] + "…"


def fmt_call(name: str, args: dict) -> str:
    inner = ", ".join(f"{k}={json.dumps(v, ensure_ascii=False)}" for k, v in args.items())
    return f"{name}({inner})"


def _add_usage(total: dict, usage) -> None:
    if usage is None:
        return
    for k, v in usage.model_dump().items():
        if isinstance(v, (int, float)):
            total[k] = total.get(k, 0) + v
        elif isinstance(v, dict):
            for k2, v2 in v.items():
                if isinstance(v2, (int, float)):
                    total[k2] = total.get(k2, 0) + v2


def run(messages: list, toolbox: Toolbox | None = None, *, model: str | None = None, max_turns: int = 10,
        verbose: bool = True, before_model: Callable[[list], list] | None = None,
        approve: Callable[[Tool, dict], bool | str] | None = None, **create_kwargs) -> Result:
    """agent 循环：调用模型 → 执行它要的工具 → 把结果放回对话 → 再调用模型，直到它不再要工具。

    - `messages` 会被就地追加（调用方可以接着用它继续对话）。
    - `before_model(messages)`：每次调用模型前，决定实际发给模型的消息（第 15 章用它做压缩）。
    - `approve(tool, args)`：执行 `risky` 工具前询问；返回 True 放行，返回字符串则把它当作拒绝理由交给模型（第 20 章）。
    - 其余参数原样传给 `chat.completions.create`（例如 `temperature`、`extra_body`）。
    """
    toolbox = toolbox or Toolbox()
    res = Result(answer=None, messages=messages)
    for turn in range(1, max_turns + 1):
        res.turns = turn
        sent = before_model(messages) if before_model else messages
        kwargs = dict(create_kwargs)
        if toolbox.tools:
            kwargs["tools"] = toolbox.schemas()
        resp = client().chat.completions.create(model=model or MODEL, messages=sent, **kwargs)
        _add_usage(res.usage, resp.usage)
        msg = resp.choices[0].message
        entry = {"role": "assistant", "content": msg.content or ""}
        # 思考模式下 DeepSeek 要求：请求带 tools 时，之前每一轮的 reasoning_content 都原样放回，
        # 否则可能返回 400（文档 guides/thinking_mode）。不带 tools 时放回也会被忽略，所以一律放回。
        reasoning = getattr(msg, "reasoning_content", None)
        if reasoning:
            entry["reasoning_content"] = reasoning
        if msg.tool_calls:
            entry["tool_calls"] = [tc.model_dump(include={"id", "type", "function"}) for tc in msg.tool_calls]
        messages.append(entry)
        if not msg.tool_calls:
            res.answer = msg.content
            return res
        for tc in msg.tool_calls:
            name = tc.function.name
            try:
                args = json.loads(tc.function.arguments or "{}")
            except json.JSONDecodeError:
                args = {"_raw": tc.function.arguments}
            t = toolbox.tools.get(name)
            verdict = approve(t, args) if (approve and t is not None and t.risky) else True
            if verdict is True:
                out = toolbox.call(name, tc.function.arguments)
            else:
                out = f"错误：这个操作没有被批准（{verdict or '审批人拒绝'}）"
            res.steps.append(Step(turn, name, args, out))
            if verbose:
                print(f"第 {turn} 轮 → {fmt_call(name, args)}  ← {short(out)}")
            messages.append({"role": "tool", "tool_call_id": tc.id, "content": out})
    res.stop = "max_turns"
    return res
