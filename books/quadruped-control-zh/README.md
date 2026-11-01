# 四足机器人控制：从关节电机到自主行走

一本可运行的中文四足机器人控制教程。沿“关节电机 → 单腿运动学 → 整机动力学 → 状态估计 →
平衡控制 → 步态与轨迹 → 行走控制器 → 感知与导航”的主线，讲清每一环的模型、公式和实现；
示例用 Python + MuJoCo 在宇树 A1 模型上实际运行，正文中的数值输出和图由脚本生成。

在线阅读：<https://winteryangwt.github.io/Awesome-Tech-Books/quadruped-control-zh/>

## 目录

- `src/`：各章 Markdown，`src/SUMMARY.md` 是目录，`src/appendix-mapping.md` 是附录 A
  “原书章节对照”（原书每一节、每一条编号公式和每一张表在本书中的位置）。
- `src/code/`：可运行示例。`src/code/quadruped/` 是第 2–11 章共用的 Python 包，
  `src/code/chNN/` 是各章示例，`src/code/assets/` 放仿真场景；A1 模型不随书分发。
- `theme/`：mdBook 的样式和页面模板。
- `LICENSE`：正文的 CC BY 4.0 许可证；`LICENSE-CODE`：示例代码的 MIT 许可证。
- 网页由仓库的 GitHub Actions 用 mdBook 0.5.1 构建，本目录不含生成的 HTML。

## 运行示例

需要 Python 3.12、MuJoCo 3.17，以及 numpy、scipy、matplotlib、osqp；C++ 示例另需 clang++ 和 Eigen。
第一次运行前先下载本书验证时使用的 A1 模型（BSD-3-Clause，版权归 Unitree Robotics）：

```bash
cd src/code
./fetch_a1.sh                 # 下载到 src/code/assets/menagerie，不进入版本库
```

然后在 `src/code/chNN/` 下运行单个示例，图写到该目录的 `figs/`：

```bash
cd src/code/ch05
PYTHONPATH=.. MUJOCO_GL=egl python3 5_1_fk.py
```

没有显示器的机器要设置 `MUJOCO_GL=egl`；依赖真实机器人或 ROS 2 的步骤在正文里分别标为
**【需要实机】** 和 **【需要 ROS 2 环境】**，这些步骤没有在本地验证。

## 许可证与说明

- 正文以 [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) 发布（见 `LICENSE`），
  示例代码以 MIT 许可证发布（见 `LICENSE-CODE`）。
- A1 模型采用 BSD-3-Clause 许可证（Unitree Robotics），需运行 `src/code/fetch_a1.sh` 下载，
  不随本书分发；模型目录中的 `LICENSE` 为准。
- 本书参考《四足机器人控制算法——建模、控制与实践》的章节结构编写，是原创教程，不是该书的复制或摘编；表格中的客观数据取自原书并注明。本书不是宇树科技或机械工业出版社的出版物，也未经其审阅或认可。
- 本书由 AI 辅助编写：正文和示例由 AI 起草，每章经过人工审阅，技术说法与原书和仿真结果逐项核对。难免仍有错误，欢迎通过 issue 指正。
