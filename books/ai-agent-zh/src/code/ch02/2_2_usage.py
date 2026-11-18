"""2.2 同一段退货政策做前缀，问两个问题，看 usage 和缓存字段。"""
# mode: run
# expect: prompt_cache_hit_tokens
# expect: cached_tokens
import os

from openai import OpenAI

from shop import Store

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")

SYSTEM = ("【青石书店客服培训·退货政策 2026-11-17】\n\n"
          "你是青石书店的客服。只根据下面的退货政策回答，用中文，简洁。\n\n" + Store.policy("returns"))
QUESTIONS = ["签收后多少天内可以退货？", "质量问题签收后多少天内可以退？把这一条完整说一遍。"]

for i, question in enumerate(QUESTIONS, 1):
    response = client.chat.completions.create(
        model=MODEL,
        messages=[{"role": "system", "content": SYSTEM}, {"role": "user", "content": question}],
        max_tokens=300,
        extra_body={"thinking": {"type": "disabled"}},
    )
    usage = response.usage
    cached = getattr(usage.prompt_tokens_details, "cached_tokens", None)
    print(f"第 {i} 次：prompt_tokens={usage.prompt_tokens} completion_tokens={usage.completion_tokens} "
          f"total_tokens={usage.total_tokens}")
    print(f"  prompt_cache_hit_tokens={usage.prompt_cache_hit_tokens} "
          f"prompt_cache_miss_tokens={usage.prompt_cache_miss_tokens} "
          f"prompt_tokens_details.cached_tokens={cached}")
    print("  回答：" + " ".join((response.choices[0].message.content or "").split())[:60])
