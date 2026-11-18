"""A.1 检查本书示例的运行环境：Python、依赖版本和环境变量是否存在。"""
# llm: none
# expect: deepseek-flash
import importlib.metadata as md
import os
import sys

PACKAGES = [
    "openai", "langgraph", "langgraph-checkpoint-sqlite", "langchain",
    "langchain-core", "langchain-openai", "langchain-deepseek", "mcp",
    "openai-agents", "anthropic",
]

print(f"Python {sys.version.split()[0]}")
for name in PACKAGES:
    try:
        version = md.version(name)
    except md.PackageNotFoundError:
        version = "未安装"
    print(f"{name:26} {version}")

print()
print("OPENAI_BASE_URL", "已设置" if os.environ.get("OPENAI_BASE_URL") else "未设置")
print("ANTHROPIC_BASE_URL", "已设置" if os.environ.get("ANTHROPIC_BASE_URL") else "未设置")
print("MODEL", os.environ.get("MODEL") or "未设置")
