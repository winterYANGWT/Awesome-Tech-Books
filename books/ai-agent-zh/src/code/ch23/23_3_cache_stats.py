# mode: run
# expect: 固定前缀
# expect: A 组
# expect: B 组
"""缓存按前缀匹配：A 组连发 6 次同样的系统提示，B 组每次在开头写当前时间，看命中率。

两组用同一段约 1.5k token 的系统提示，都带同一套工具定义；只发请求读 usage，不看回答。
DeepSeek 的 `prompt_cache_hit_tokens` 是这次请求里命中缓存的输入 token 数。
"""
import os

from openai import OpenAI

from shop import Store
from shop.tools import SUPPORT_PROMPT, support_tools

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")
N = 6


def system_prompt():
    body = "\n\n".join(Store.policy(name) for name in Store.policy_names())
    return SUPPORT_PROMPT + "\n\n【店铺政策原文】\n" + body


def ask(system, question, tools):
    resp = client.chat.completions.create(
        model=MODEL, messages=[{"role": "system", "content": system},
                               {"role": "user", "content": question}],
        tools=tools, max_tokens=32, extra_body={"thinking": {"type": "disabled"}})
    u = resp.usage.model_dump()
    return u["prompt_tokens"], u.get("prompt_cache_hit_tokens", 0)


def main():
    base = system_prompt()
    tools = support_tools(Store()).schemas()
    question = "签收后多久能退货？"
    print(f"系统提示 {len(base)} 字，工具 {len(tools)} 个；每组 {N} 次请求")

    for label, make_system in [
        ("A 组 固定前缀", lambda i: base),
        ("B 组 每次换时间戳", lambda i: f"当前时间：2026-11-17 14:03:{i:02d}。\n" + base),
    ]:
        prompt_sum = hit_sum = 0
        for i in range(1, N + 1):
            prompt, hit = ask(make_system(i), question, tools)
            prompt_sum += prompt
            hit_sum += hit
            print(f"  {i}  prompt={prompt:,}  hit={hit:,}  rate={hit / prompt:.1%}")
        print(f"{label}：6 次命中 {hit_sum:,} / {prompt_sum:,} = {hit_sum / prompt_sum:.1%}")


if __name__ == "__main__":
    main()
