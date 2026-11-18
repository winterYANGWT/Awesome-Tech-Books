# 18.6 子 agent 把结果写成 findings/*.json；一条线失败时主 agent 读其余文件并写 gap。
# mode: run
# llm: none
# expect: findings/
import json
from pathlib import Path

from ch18.lib.fixtures import FINDINGS

OUT = Path("findings")


def write(name: str, result: dict) -> Path:
    path = OUT / f"{name}.json"
    path.write_text(json.dumps(result, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    return path


def run_reviews() -> dict:
    """模拟评论线超时：不抛到主流程，只返回 failed 和 gap。"""
    raise TimeoutError("评论服务 20 秒没有返回")


def main() -> None:
    OUT.mkdir(exist_ok=True)
    paths = {}
    for name in ("sql", "policy"):
        paths[name] = write(name, FINDINGS[name])
        print(f"写出 {paths[name]}  status=ok  findings={len(FINDINGS[name]['findings'])}")
    try:
        result = run_reviews()
    except TimeoutError as e:
        result = {"status": "failed", "findings": [],
                  "gaps": [f"评论线失败：{e}；B07、B19、B12 的评论都缺，B12 的样本缺口也要在恢复后补"]}
    paths["reviews"] = write("reviews", result)
    print(f"写出 {paths['reviews']}  status=failed  gaps={len(result['gaps'])}")

    # 主 agent 只读文件，不接收子 agent 的全部对话
    loaded = {p.stem: json.loads(p.read_text(encoding="utf-8")) for p in sorted(OUT.glob("*.json"))}
    ok = [k for k, v in loaded.items() if v["status"] == "ok"]
    bad = [k for k, v in loaded.items() if v["status"] != "ok"]
    print(f"\n主 agent 读 {len(loaded)} 个文件：ok {len(ok)}（{', '.join(ok)}），failed {len(bad)}（{', '.join(bad)}）")

    sql = loaded["sql"]["findings"]
    policy = loaded["policy"]["findings"]
    print("\n报告（只用读得到的文件）：")
    print(f"- Q3 退货率 18.3%（38/208），Q4 8.2%（8/98）[数据：{sql[0]['source']}]")
    print(f"- 青川出版社的 B07、B12、B19 贡献 20 笔缺页/装订 [{sql[2]['source']}]")
    print(f"- 退货期限 2026-07-01 起 7→15 天；2026-08-01 起两广海南改迅达 "
          f"[政策：{policy[0]['source']}；{policy[2]['source']}]")
    print("- 评论线失败，B07、B19、B12 的评论都缺；B07/B12/B19 的逐书批次拆分也还缺 "
          "[数据：returns / books]")
    for gap in loaded["reviews"]["gaps"] + loaded["sql"]["gaps"]:
        print(f"gap：{gap}")
    print(f"\n文件都在 {OUT}/ 下；失败的那份也有 status 和 gaps，主 agent 不需要重跑")


if __name__ == "__main__":
    main()
