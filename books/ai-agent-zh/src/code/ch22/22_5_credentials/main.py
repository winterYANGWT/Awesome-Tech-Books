# 22.5 沙箱读环境变量和 .env 都拿不到 token；通过 MCP 代理调用，代理从 vault 取 token。
# mode: run
# llm: none
# expect: sandbox env keys=\[\]
# expect: token in sandbox=False
# expect: proxy status=200
"""vault 和代理在 harness 进程；沙箱子进程只有一段代码和一个 session id。"""
from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path

from proxy import ProxyServer
from vault import Vault

SANDBOX_CODE = r'''
import json, os, pathlib, urllib.error, urllib.request
BASE = "__BASE__"
print("sandbox env keys=" + str(sorted(k for k in os.environ if not k.startswith("LC_"))))
found = any("tok_" in value for value in os.environ.values())
for name in (".env", "../.env", "../../.env"):
    p = pathlib.Path(name)
    if p.exists() and "tok_" in p.read_text(errors="ignore"):
        found = True
print(f"token in sandbox={found}")
try:
    with urllib.request.urlopen(BASE + "/service", timeout=2) as r:
        print(f"direct service without token={r.status}")
except urllib.error.HTTPError as e:
    print(f"direct service without token={e.code}")
req = urllib.request.Request(BASE + "/tool", headers={"X-Session": "S-T06"})
with urllib.request.urlopen(req, timeout=3) as r:
    body = r.read().decode()
print(f"proxy status={r.status} body={body}")
'''


def main() -> None:
    vault = Vault()
    token = vault.get("mcp:orders")
    with ProxyServer(vault) as proxy, tempfile.TemporaryDirectory() as tmp:
        code = SANDBOX_CODE.replace("__BASE__", proxy.base)
        cp = subprocess.run([sys.executable, "-I", "-c", code], cwd=tmp, env={},
                            capture_output=True, text=True, timeout=15)
        if cp.returncode != 0:
            raise SystemExit(cp.stderr)
        print(cp.stdout.rstrip())
        print(f"父进程检查：token 没有出现在沙箱输出里={token not in cp.stdout}")
        print(f"vault 在沙箱外：token 长度 {len(token)}（不打印内容）")


if __name__ == "__main__":
    main()
