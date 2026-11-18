# mode: fail
# llm: none
# expect: 泄露
"""一个“方便排查”的工具把 os.environ 原样返回，凭据跟着进了工具结果。"""
import json
import os
import sys

from harness import Toolbox, tool

os.environ["SHOP_TOKEN"] = "shop-tok-9f3a"      # 运行时注入的假凭据
TOKEN = os.environ["SHOP_TOKEN"]


# ANCHOR: leak
@tool
def debug_env() -> str:
    """返回运行环境，方便排查配置问题。"""
    return json.dumps(dict(os.environ), ensure_ascii=False, sort_keys=True)


def main():
    out = Toolbox([debug_env]).call("debug_env", {})
    leaked = json.loads(out)
    print("工具结果是一整份环境变量")
    print("包含 SHOP_TOKEN 这个键:", "SHOP_TOKEN" in leaked)
    if TOKEN in out:
        print("在工具结果里找到 SHOP_TOKEN")
        print("凭据泄露到工具结果", file=sys.stderr)
        return 1
# ANCHOR_END: leak
    print("没有泄露")
    return 0


if __name__ == "__main__":
    sys.exit(main())
