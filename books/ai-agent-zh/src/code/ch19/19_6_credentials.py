# 19.6 凭据不进模型能看到的地方：token 只在工具进程的环境里。
# mode: run
# llm: none
# expect: 工具进程读到 SHOP_TOKEN： True
# expect: sandbox 读 .env → FileNotFoundError
# expect: tool_schema_has_token = False
# expect: api_calls=0
import json
import pathlib
import subprocess
import sys
import tempfile

from shop import Store
from shop.tools import support_tools

FAKE = "tok_test_7f3a"                             # 假的：只用来检查字符串有没有出现在别处
store = Store()
box = support_tools(store)

# 模型侧能看到的：工具 schema。
tool_schema_has_token = FAKE in json.dumps(box.schemas(), ensure_ascii=False)

TOOL_ENV = {"PATH": "/usr/bin:/bin", "HOME": "/sandbox", "LANG": "C.UTF-8",
            "PYTHONIOENCODING": "utf-8", "SHOP_TOKEN": FAKE}
SANDBOX_ENV = {"PATH": "/usr/bin:/bin", "HOME": "/sandbox", "LANG": "C.UTF-8",
               "PYTHONIOENCODING": "utf-8"}

TOOL_CODE = "import os; t = os.environ.get('SHOP_TOKEN', ''); print('工具进程读到 SHOP_TOKEN：', bool(t))"
SANDBOX_CODE = """
import pathlib
root = pathlib.Path.cwd()
def read_file(path):
    full = (root / path).resolve()
    if root.resolve() not in full.parents and full != root.resolve():
        raise PermissionError("路径越出 sandbox")
    return full.read_text(encoding="utf-8")
try:
    read_file(".env")
except FileNotFoundError:
    print("sandbox 读 .env → FileNotFoundError")
try:
    read_file("../.env")
except PermissionError:
    print("sandbox 读 ../.env → PermissionError")
"""

with tempfile.TemporaryDirectory() as d:
    root = pathlib.Path(d)
    (root / ".env").write_text(f"SHOP_TOKEN={FAKE}\n", encoding="utf-8")
    sandbox = root / "sandbox"
    sandbox.mkdir()

    # 工具进程从自己的环境变量读 token。
    tool = subprocess.run([sys.executable, "-c", TOOL_CODE], cwd=root, env=TOOL_ENV,
                          capture_output=True, text=True, check=True)
    print(tool.stdout.strip())

    # sandbox 只拿到最小的环境变量，文件工具也只允许自己的目录。
    sand = subprocess.run([sys.executable, "-c", SANDBOX_CODE], cwd=sandbox, env=SANDBOX_ENV,
                          capture_output=True, text=True, check=True)
    print(sand.stdout.strip())

print("tool_schema_has_token =", tool_schema_has_token)
print("api_calls=0")
