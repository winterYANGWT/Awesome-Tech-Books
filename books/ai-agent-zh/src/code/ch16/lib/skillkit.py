"""第 16 章示例共用的最小 Skill 加载器：发现、解析 frontmatter、目录和 token 估算。"""
from __future__ import annotations

import pathlib
import re
from dataclasses import dataclass, field

import tiktoken
import yaml

ENC = tiktoken.get_encoding("cl100k_base")
NAME_RE = re.compile(r"^[a-z0-9]+(?:-[a-z0-9]+)*$")


def tokens(text: str) -> int:
    """用 cl100k_base 估算 token；真实客户端的分词器不同，这里只用来比较大小。"""
    return len(ENC.encode(text))


@dataclass
class Skill:
    name: str
    description: str
    body: str
    location: pathlib.Path
    files: list[str] = field(default_factory=list)
    meta: dict = field(default_factory=dict)

    @property
    def body_tokens(self) -> int:
        return tokens(self.body)

    @property
    def catalog_tokens(self) -> int:
        return tokens(f"{self.name}: {self.description}")


def _quote_colon_lines(raw: str) -> str:
    """把 `description: 处理退款: 计算金额` 这种未加引号的冒号值包成字符串。"""
    fixed = []
    for line in raw.splitlines():
        m = re.match(r"^([A-Za-z0-9_-]+):\s*(.+)$", line)
        if m and ": " in m.group(2) and not m.group(2).lstrip().startswith(('"', "'", "|", ">")):
            fixed.append(f'{m.group(1)}: "{m.group(2).replace(chr(34), chr(92) + chr(34))}"')
        else:
            fixed.append(line)
    return "\n".join(fixed)


def split_frontmatter(text: str) -> tuple[dict, str, list[str]]:
    """返回 (frontmatter, 正文, 警告)。只处理 `---` 开头、`---` 结束的常见写法。"""
    warn: list[str] = []
    if not text.startswith("---\n"):
        raise ValueError("SKILL.md 没有 YAML frontmatter")
    end = text.find("\n---", 4)
    if end < 0:
        raise ValueError("frontmatter 没有结束的 ---")
    raw = text[4:end]
    body = text[end + 4:].lstrip("\n")
    try:
        meta = yaml.safe_load(raw) or {}
    except yaml.YAMLError as e:
        meta = yaml.safe_load(_quote_colon_lines(raw)) or {}
        warn.append(f"YAML 有未加引号的冒号（{str(e).splitlines()[0]}），已按字符串回退")
    if not isinstance(meta, dict):
        raise ValueError("frontmatter 不是键值对")
    return meta, body, warn


def load_skill(path: pathlib.Path) -> tuple[Skill | None, list[str]]:
    """加载一个 Skill 目录；缺 description 时跳过（规范允许客户端这样做）。"""
    path = pathlib.Path(path)
    warn: list[str] = []
    try:
        meta, body, w = split_frontmatter((path / "SKILL.md").read_text(encoding="utf-8"))
        warn += w
    except (OSError, ValueError) as e:
        return None, [f"跳过 {path.name}：{e}"]

    name = str(meta.get("name") or "").strip()
    description = str(meta.get("description") or "").strip()
    if not description:
        return None, [f"跳过 {path.name}：description 缺失"]
    if not name:
        return None, [f"跳过 {path.name}：name 缺失"]
    if not NAME_RE.fullmatch(name) or len(name) > 64:
        warn.append(f"{name} 不符合 name 规范（小写字母/数字/连字符，1–64 字符），仍然加载")
    if name != path.name:
        warn.append(f"{name} 与目录名 {path.name} 不一致，仍然加载")
    if len(description) > 1024:
        warn.append(f"{name} 的 description 超过 1024 字符，客户端可能拒绝")

    files = sorted(str(p.relative_to(path)) for p in path.rglob("*")
                   if p.is_file() and p.name != "SKILL.md" and "__pycache__" not in p.parts)
    return Skill(name, description, body, path / "SKILL.md", files, meta), warn


def discover(roots: list[pathlib.Path]) -> tuple[list[Skill], list[str]]:
    """按顺序扫描；同名时先找到的覆盖后面的，并打印一条警告。"""
    found: dict[str, Skill] = {}
    warn: list[str] = []
    for root in roots:
        if not root.is_dir():
            continue
        for child in sorted(root.iterdir()):
            if not (child / "SKILL.md").is_file():
                continue
            skill, w = load_skill(child)
            warn += w
            if skill is None:
                continue
            if skill.name in found:
                warn.append(f"{skill.name} 在 {root} 里被同名 Skill 覆盖")
                continue
            found[skill.name] = skill
    return [found[k] for k in sorted(found)], warn


def catalog(skills: list[Skill]) -> str:
    lines = ["<available_skills>"]
    for s in skills:
        lines.append(f"- {s.name}: {s.description}")
    lines.append("</available_skills>")
    return "\n".join(lines)
