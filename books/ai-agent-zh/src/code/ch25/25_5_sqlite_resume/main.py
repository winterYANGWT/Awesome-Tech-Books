# 25.5 两个子进程共享 state.db：进程 1 中断退出，进程 2 新开数据库恢复。
# mode: run
# llm: none
# expect: approved=True
"""`SqliteSaver` 把检查点写到文件；`InMemorySaver` 在进程退出时就没了。"""
import pathlib
import subprocess
import sys

HERE = pathlib.Path(__file__).resolve().parent
DB = HERE / "state.db"
DB.unlink(missing_ok=True)

for script in ("phase1.py", "phase2.py"):
    cp = subprocess.run([sys.executable, script], cwd=HERE, capture_output=True, text=True)
    print(cp.stdout.strip())
    if cp.returncode != 0:
        print(cp.stderr.strip(), file=sys.stderr)
        raise SystemExit(cp.returncode)
print(f"state.db 在两次进程之间保留：{DB.exists()}")
