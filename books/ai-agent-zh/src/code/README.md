# 本书的示例代码

在这个目录（`code/`）里运行：

```bash
python3.12 -m venv .venv && . .venv/bin/activate
pip install -r requirements.txt
export PYTHONPATH=$PWD                       # 让示例能 import shop、harness
export OPENAI_BASE_URL=https://api.deepseek.com
export OPENAI_API_KEY=sk-...                 # 你的 DeepSeek key（或其他 OpenAI 兼容服务的 key 和地址）
export MODEL=deepseek-flash                  # 换成你用的模型名
python ch03/3_2_tool_loop.py
```

- `shop/`：贯穿全书的示例“青石书店”（数据、政策文档、客服工具）。
- `harness/`：第 3 章写出来的最小 agent 循环，后面各章直接用。
- `chNN/`：各章的示例。`*.out` 是书里展示的那一次运行的输出，`*.cassette.jsonl` 是那次运行的全部模型请求和响应。
  模型每次的回答都不同，你自己运行得到的结果会和书里不一样。
