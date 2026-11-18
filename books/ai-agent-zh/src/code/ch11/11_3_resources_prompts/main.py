# llm: none
# expect: shop://policies/returns
# expect: refund_guidance

"""列资源模板、读一份政策、列提示并取一个话术。"""

import asyncio
import sys
from pathlib import Path

from mcp import Client
from mcp.client.stdio import StdioServerParameters

CODE = Path(__file__).resolve().parents[2]
SERVER = Path(__file__).with_name("server.py")


async def main() -> None:
    params = StdioServerParameters(command=sys.executable, args=[str(SERVER)],
                                   env={"PYTHONPATH": str(CODE)}, cwd=str(CODE))
    async with Client(params) as client:
        print("resources/list:", len((await client.list_resources()).resources))
        templates = (await client.list_resource_templates()).resource_templates
        print("templates:", len(templates))
        for t in templates:
            print("  uriTemplate:", t.uri_template, "mimeType:", t.mime_type)

        resource = await client.read_resource("shop://policies/returns")
        first = [line for line in resource.contents[0].text.splitlines() if line.strip()]
        print("read:", resource.contents[0].uri, resource.contents[0].mime_type)
        print("ttlMs:", resource.ttl_ms, "cacheScope:", resource.cache_scope)
        print("text:", first[0], "|", first[1])

        prompts = (await client.list_prompts()).prompts
        for p in prompts:
            print("prompt:", p.name, [(a.name, a.required) for a in p.arguments])

        got = await client.get_prompt("refund_guidance", {"order_id": "A1659", "amount": "82"})
        print("messages:", [(m.role, m.content.text) for m in got.messages])


if __name__ == "__main__":
    asyncio.run(main())
