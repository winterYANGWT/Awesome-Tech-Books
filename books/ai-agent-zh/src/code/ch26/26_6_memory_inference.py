# 26.6 统计示例：同一句中性输入，两种提示各 8 次，统计推断出的偏好。
# mode: run
# env: N=8
# expect: A 组:
# expect: B 组:

import os

from langchain_core.messages import HumanMessage, SystemMessage
from langchain_openai import ChatOpenAI
from pydantic import BaseModel, Field

MODEL = os.environ.get("MODEL", "deepseek-flash")
N = int(os.environ.get("N", "8"))
SAID = "书收到了，包装很好。"

PROMPTS = {
    "A": (
        "只记录顾客明确说出的长期偏好。只有当顾客说“我喜欢”“我偏好”“我以后要”"
        "这类明确表达时才填写 preference；评价包装、物流等一次性感受不算偏好，"
        "preference 留空字符串。"
    ),
    "B": (
        "请尽量补全顾客偏好，让档案更有用。顾客没有直接说、但从这句话可以合理推断的"
        "偏好，也写进 preference。"
    ),
}


class Profile(BaseModel):
    preference: str = Field(description="顾客偏好；没有就填空字符串")


extractor = ChatOpenAI(
    model=MODEL, extra_body={"thinking": {"type": "disabled"}}
).with_structured_output(Profile, method="function_calling")

counts = {}
for group, system in PROMPTS.items():
    print(f"{group} 组（{'只记明确事实' if group == 'A' else '尽量补全偏好'}）")
    hits = 0
    for i in range(1, N + 1):
        profile = extractor.invoke([
            SystemMessage(content=f"{system}\n这是第 {i} 次抽取。"),
            HumanMessage(content=SAID),
        ])
        wrote = bool(profile.preference.strip())
        hits += wrote
        print(f"  {i}: {'写了 preference' if wrote else 'preference 为空'}")
    counts[group] = hits
    print(f"{group} 组: {hits}/{N} 写入了未声明偏好")

print("两组只改了系统提示，用户消息相同。")
