# llm: none
# expect: server_info
# expect: server_capabilities
# expect: PYTHONPATH

"""连接诊断：协商版本、server 信息、能力，以及一份可以交给 host 的配置。"""

import asyncio
import json
import sys
from pathlib import Path

from mcp import Client
from mcp.client.stdio import StdioServerParameters

CODE = Path(__file__).resolve().parents[1]
SERVER = CODE / "ch11" / "11_2_shop_server" / "server.py"


def stable(value):
    """把解释器和代码目录换成 $PYTHON、$CODE，输出才能回放。"""
    if isinstance(value, Path):
        value = str(value)
    if isinstance(value, str):
        return value.replace(sys.executable, "$PYTHON").replace(str(CODE), "$CODE")
    if isinstance(value, list):
        return [stable(v) for v in value]
    if isinstance(value, dict):
        return {k: stable(v) for k, v in value.items()}
    return value


async def main() -> None:
    # ANCHOR: config
    config = {"command": sys.executable, "args": [str(SERVER)],
              "env": {"PYTHONPATH": str(CODE)}, "cwd": str(CODE)}
    # ANCHOR_END: config
    params = StdioServerParameters(command=config["command"], args=config["args"],
                                   env=config["env"], cwd=config["cwd"])
    async with Client(params) as client:
        print("protocol_version:", client.protocol_version)
        print("server_info:", json.dumps(client.server_info.model_dump(exclude_none=True), ensure_ascii=False))
        caps = client.server_capabilities.model_dump(by_alias=True, exclude_none=True)
        print("server_capabilities:", json.dumps(caps, ensure_ascii=False, sort_keys=True))
        print("tools:", len((await client.list_tools()).tools),
              "templates:", len((await client.list_resource_templates()).resource_templates),
              "prompts:", len((await client.list_prompts()).prompts))

    shown = stable(config)
    print("host config:")
    print("  command:", shown["command"])
    print("  args:", shown["args"])
    print("  env:", shown["env"])
    print("  cwd:", shown["cwd"])
    blob = json.dumps(shown)
    print("  has_secret:", any(word in blob.upper() for word in ("KEY", "TOKEN", "SECRET")))


if __name__ == "__main__":
    asyncio.run(main())
