#!/usr/bin/env bash
# 下载本书使用的 Unitree A1 模型（MuJoCo Menagerie，BSD-3-Clause，版权归 Unitree Robotics），
# 固定在本书验证时的提交，放到 code/assets/menagerie/unitree_a1/。
set -euo pipefail
cd "$(dirname "$0")"
COMMIT=f3b6013629c7b94f8ee93fd977b161fa8350f9bc
DEST=assets/menagerie
if [ ! -f "$DEST/unitree_a1/scene.xml" ]; then
    rm -rf "$DEST"
    git clone -q --filter=blob:none --sparse https://github.com/google-deepmind/mujoco_menagerie.git "$DEST"
    git -C "$DEST" sparse-checkout set unitree_a1
    git -C "$DEST" checkout -q "$COMMIT"
fi
echo "A1 model: $PWD/$DEST/unitree_a1/scene.xml"
