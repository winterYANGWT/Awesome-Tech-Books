# 22.4 本地 sandbox 的四条边界：正常、超时、目录逃逸、输出截断。
# mode: run
# llm: none
# expect: timeout
# expect: denied
# expect: truncated
"""用子进程模拟容器：cwd 是临时目录，env 是空的，只回一段字符串。"""
from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path

LIMIT = 200


class Sandbox:
    def __init__(self, workdir: Path):
        self.workdir = workdir

    # ANCHOR: execute
    def execute(self, name: str, input: str, timeout: float = 0.5) -> str:
        if name != "run_python":
            return f"error：没有 {name} 这个执行环境"
        if ".." in input:
            return "denied：代码里出现 ..，可能是想读工作目录以外的文件"
        try:
            cp = subprocess.run([sys.executable, "-I", "-c", input], cwd=self.workdir,
                                timeout=timeout, env={}, capture_output=True, text=True)
        except subprocess.TimeoutExpired:
            return f"timeout：超过 {timeout:g}s，子进程已杀掉"
        out = (cp.stdout or "").rstrip("\n")
        err = (cp.stderr or "").strip().splitlines()
        if len(out) > LIMIT:
            return f"truncated：{LIMIT}/{len(out)} 字符\n{out[:LIMIT]}"
        if cp.returncode != 0:
            return f"error：exit={cp.returncode} {err[-1] if err else ''}".strip()
        return f"ok：exit=0 stdout={out}"
    # ANCHOR_END: execute


def main() -> None:
    with tempfile.TemporaryDirectory() as root:
        root = Path(root)
        (root / "secret.txt").write_text("GITHUB_TOKEN=tok_demo\n", encoding="utf-8")
        work = root / "inner"
        work.mkdir()
        box = Sandbox(work)
        cases = [
            ("正常", "run_python", "print('82')"),
            ("死循环", "run_python", "while True:\n    pass"),
            ("读 ../secret", "run_python", "print(open('../secret.txt').read())"),
            ("输出 10KB", "run_python", "print('A' * 10000)"),
        ]
        for i, (label, name, code) in enumerate(cases, 1):
            result = box.execute(name, code)
            print(f"{i} {label}：{result.splitlines()[0]}")
        print(f"工作目录里的文件：{sorted(p.name for p in work.iterdir()) or '[]'}")
        # 对照：去掉 execute() 里的 .. 检查，同一段代码就能读到 root/secret.txt。
        cp = subprocess.run([sys.executable, "-I", "-c", "print(open('../secret.txt').read().strip())"],
                            cwd=work, env={}, capture_output=True, text=True)
        print(f"去掉 .. 检查后：{cp.stdout.strip()}")
        print(f"secret.txt 在 cwd 外：{(root / 'secret.txt').exists()}")


if __name__ == "__main__":
    main()
