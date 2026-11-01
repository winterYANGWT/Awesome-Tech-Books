# expect: Python: 3\.12
# expect: MuJoCo: 3\.17
# expect: 关节数: 12
# expect: 执行器数: 12
# expect: 总质量: 12\.453 kg
# expect: 渲染: 640x480
"""One-shot environment check for the whole book.

It prints the versions that the examples were tested with, loads the A1,
lists the twelve joints, and renders one 640x480 frame.  Every step is
checked; a missing package, a missing model or a black frame exits with a
non-zero status so that a broken machine is noticed before Chapter 3.

The PNG is written to ``$FIG_DIR/1_3_env_check.png`` (default ``figs/``).
"""

from __future__ import annotations

import os
import platform
import sys

# Mesa's shader cache is not writable in some sandboxes; disabling it keeps
# stderr clean without changing the rendered image.
os.environ.setdefault("MESA_SHADER_CACHE_DISABLE", "true")

import numpy as np

import mujoco
import scipy
import osqp

from quadruped import A1Sim
from quadruped.plotting import fig_dir

import matplotlib.image as mpimg

WIDTH, HEIGHT = 640, 480


def main() -> int:
    print(f"Python: {platform.python_version()}")
    print(f"MuJoCo: {mujoco.__version__}")
    print(f"numpy: {np.__version__}")
    print(f"scipy: {scipy.__version__}")
    print(f"osqp: {osqp.__version__}")

    # ANCHOR: check
    try:
        sim = A1Sim()
    except Exception as exc:  # missing model or missing EGL
        print(f"检查失败: 无法加载 A1: {exc}")
        return 1

    try:
        model = sim.model
        if model.nq != 19 or model.nv != 18:
            print(f"检查失败: nq/nv = {model.nq}/{model.nv}，期望 19/18")
            return 1
        if model.nu != 12:
            print(f"检查失败: 执行器数 = {model.nu}，期望 12")
            return 1
        if len(sim.joint_names) != 12:
            print(f"检查失败: 关节名 = {len(sim.joint_names)} 个，期望 12")
            return 1

        mass = float(np.sum(model.body_mass))
        print(f"模型自由度: nq={model.nq}, nv={model.nv}, "
              f"铰链关节数: 12（1 个浮动基座 + 12 个铰链；model.njnt={model.njnt}）")
        print(f"执行器数: {model.nu}（每个关节一个位置执行器）")
        print(f"总质量: {mass:.3f} kg")

        base, joints = sim.get_state()
        print(f"初始机身高度: {base.position[2]:.4f} m")
        print("12 个关节（顺序：FR, FL, RR, RL；每腿 hip/thigh/calf）:")
        for i, name in enumerate(sim.joint_names):
            print(f"  {i:2d}  {name:<15s} q={joints.q[i]: .4f} rad")

        # The model's built-in "track" camera looks straight down in this
        # MuJoCo version; a free camera gives a reproducible three-quarter
        # view of the whole machine.
        cam = mujoco.MjvCamera()
        cam.type = mujoco.mjtCamera.mjCAMERA_FREE
        cam.lookat[:] = [0.0, 0.0, 0.22]
        cam.distance = 1.0
        cam.azimuth = 135.0
        cam.elevation = -12.0
        renderer = mujoco.Renderer(model, HEIGHT, WIDTH)
        renderer.update_scene(sim.data, camera=cam)
        img = renderer.render().copy()
        renderer.close()
        if img.shape != (HEIGHT, WIDTH, 3):
            print(f"检查失败: 渲染尺寸 = {img.shape}")
            return 1
        if float(img.std()) < 5.0:
            print(f"检查失败: 渲染图像近似纯色，std={img.std():.2f}")
            return 1
        out = fig_dir() / "1_3_env_check.png"
        out.parent.mkdir(parents=True, exist_ok=True)
        mpimg.imsave(out, img)
        if not out.exists() or out.stat().st_size < 10_000:
            print(f"检查失败: PNG 未写出或过小: {out}")
            return 1
        print(f"渲染: {WIDTH}x{HEIGHT}, 像素标准差 {img.std():.1f}")
        print("图已保存: figs/1_3_env_check.png")
    # ANCHOR_END: check
    except Exception as exc:
        print(f"检查失败: {type(exc).__name__}: {exc}")
        return 1
    finally:
        sim.close()

    print("环境自检: 通过")
    return 0


if __name__ == "__main__":
    sys.exit(main())
