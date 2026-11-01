# expect: 地址检查: 192\.168\.123\.162 在 192\.168\.123\.0/24 内: 是
# expect: 与机器人已有地址冲突: 否
# expect: 可以用于用户计算机: 是
# expect: network:
# expect: version: 2
# expect: 检查清单: ping -> ssh -> scp -> 启动控制器
"""Generate a netplan file, the expected routes and a link checklist (3.6).

The script never touches the network.  It takes an interface name and an IPv4
address, checks the address against the A1 address plan, prints a netplan
YAML fragment, the routes the machine should have, and the ping -> ssh -> scp
-> launch checklist.  It also draws the A1/Go1 communication diagrams, the
back-panel port map, and the terminal-style figures used in section 3.6.
"""

from __future__ import annotations

import ipaddress
import logging

from quadruped.plotting import savefig

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

import matplotlib.pyplot as plt  # noqa: E402
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch, Rectangle  # noqa: E402

INTERFACE = "enp3s0"
HOST_IP = "192.168.123.162/24"
ROBOT_IP = "192.168.123.161"
ROUTER_IP = "192.168.123.1"
RESERVED = {
    "192.168.123.10": "主控板（普通模式/底层通信）",
    "192.168.123.12": "A1 树莓派/TX2（图传/SLAM）",
    "192.168.123.13": "Go1 Nano1（头部相机）",
    "192.168.123.14": "Go1 Nano2（左右相机）",
    "192.168.123.15": "Go1 Nano3（主 Nano）",
    "192.168.123.161": "Go1 树莓派 / A1 UP Board（运动模式）",
    "192.168.123.1": "路由器 LAN 口",
}

MONO = "monospace"


def box(ax, x, y, w, h, text, color="#eceff1", fs=8.0):
    ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.04",
                                facecolor=color, edgecolor="0.3", lw=1.0))
    ax.text(x + w / 2, y + h / 2, text, ha="center", va="center", fontsize=fs)


def link(ax, p0, p1, text=None, color="0.25", both=True, rad=0.0, tx=0, ty=0):
    style = "<|-|>" if both else "-|>"
    ax.add_patch(FancyArrowPatch(p0, p1, arrowstyle=style, mutation_scale=8,
                                 lw=1.0, color=color,
                                 connectionstyle=f"arc3,rad={rad}"))
    if text:
        ax.text((p0[0] + p1[0]) / 2 + tx, (p0[1] + p1[1]) / 2 + ty, text,
                fontsize=6.8, color=color, ha="center")


def draw_a1_network(ax) -> None:
    ax.set_xlim(0, 12.6)
    ax.set_ylim(0, 8.0)
    ax.axis("off")
    ax.set_title("A1 通信框架：普通模式走主控板，运动模式走 UP Board", fontsize=10)
    ax.add_patch(Rectangle((0.2, 0.3), 9.6, 7.4, fill=False, ls="--",
                           edgecolor="0.5"))
    ax.text(0.45, 0.55, "机器人内部硬件", fontsize=8, color="0.4")

    box(ax, 0.5, 5.4, 3.0, 1.35,
        "主控板 192.168.123.10\n普通模式 / 底层通信", "#e3f2fd")
    box(ax, 0.5, 2.5, 3.0, 0.95, "12 个关节电机\nRS-485", "#fff8e1")
    box(ax, 0.5, 6.95, 2.2, 0.55, "无线遥控器", "#f3e5f5", fs=7.5)
    box(ax, 4.0, 3.6, 1.6, 0.75, "交换机", "#eceff1")
    box(ax, 6.4, 5.4, 3.2, 1.25,
        "树莓派 / TX2\n192.168.123.12\n图传 / SLAM", "#e8f5e9", fs=7.6)
    box(ax, 6.4, 2.4, 3.2, 1.25,
        "UP Board\n192.168.123.161\n运动模式 / unitree_legged_sdk",
        "#ede7f6", fs=7.6)
    box(ax, 10.2, 3.3, 2.1, 1.2,
        "用户计算机\n192.168.123.xxx", "#fffde7", fs=7.6)

    link(ax, (0.5, 5.9), (0.5, 6.95), both=False)
    ax.text(0.6, 6.35, "无线数传", fontsize=6.8, color="0.3")
    link(ax, (2.0, 5.4), (2.0, 3.45), both=False)
    ax.text(2.1, 4.45, "RS-485", fontsize=6.8, color="0.3")
    link(ax, (3.5, 6.05), (4.4, 4.35), "以太网", tx=0.1, ty=0.22)
    link(ax, (4.4, 4.35), (6.4, 6.0), "以太网", tx=0.0, ty=0.22)
    link(ax, (4.4, 3.65), (6.4, 3.05), "以太网", tx=0.0, ty=-0.22)
    link(ax, (5.6, 3.9), (10.2, 3.9), "以太网", tx=0.0, ty=0.18)


def draw_go1_network(ax) -> None:
    ax.set_xlim(0, 14.5)
    ax.set_ylim(0, 8.2)
    ax.axis("off")
    ax.set_title("Go1 通信框架：主控板、树莓派、三台 Nano 与 WiFi 模块", fontsize=10)
    ax.add_patch(Rectangle((0.2, 0.3), 11.2, 7.6, fill=False, ls="--",
                           edgecolor="0.5"))
    ax.text(0.45, 0.52, "机器人内部硬件", fontsize=8, color="0.4")

    box(ax, 0.5, 5.5, 2.7, 1.3, "主控板 192.168.123.10\n普通模式 / 底层通信",
        "#e3f2fd", fs=7.6)
    box(ax, 0.5, 2.3, 2.7, 0.9, "12 个关节电机\nRS-485", "#fff8e1", fs=7.6)
    box(ax, 0.5, 7.0, 2.0, 0.5, "无线遥控器", "#f3e5f5", fs=7.2)
    box(ax, 3.8, 3.55, 1.3, 0.65, "交换机", "#eceff1", fs=7.6)
    box(ax, 5.8, 6.0, 3.3, 0.85,
        "树莓派 192.168.123.161\n运动模式 / unitree_legged_sdk", "#ede7f6",
        fs=7.2)
    box(ax, 5.8, 4.80, 3.3, 0.75, "Nano1 192.168.123.13\n头部相机图传/点云",
        "#e8f5e9", fs=7.2)
    box(ax, 5.8, 3.65, 3.3, 0.75, "Nano2 192.168.123.14\n左右相机图传/点云",
        "#e8f5e9", fs=7.2)
    box(ax, 5.8, 2.50, 3.3, 0.75, "Nano3 192.168.123.15\n主 Nano / AI / 下方相机",
        "#e8f5e9", fs=7.2)
    box(ax, 9.70, 6.55, 1.55, 0.70, "WiFi 模块\n192.168.12.1", "#e0f7fa",
        fs=7.0)
    box(ax, 12.00, 6.60, 2.15, 0.60, "APP\n192.168.12.xxx", "#f3e5f5", fs=7.2)
    box(ax, 12.00, 4.20, 2.15, 0.90, "用户计算机\n192.168.123.xxx", "#fffde7",
        fs=7.4)

    # Left side: the main board, the motors and the remote.
    link(ax, (0.5, 6.0), (0.5, 7.0), both=False)
    ax.text(0.6, 6.5, "无线数传", fontsize=6.5, color="0.3")
    link(ax, (2.0, 5.5), (2.0, 3.2), both=False)
    ax.text(2.1, 4.35, "RS-485", fontsize=6.5, color="0.3")
    link(ax, (3.2, 6.1), (4.1, 4.20), "以太网", tx=0.1, ty=0.2)

    # Ethernet bus to the four computers.
    ax.plot([5.1, 5.45], [3.85, 3.85], color="0.25", lw=1.0)
    ax.plot([5.45, 5.45], [2.85, 6.40], color="0.25", lw=1.0)
    for y in (6.40, 5.15, 4.00, 2.85):
        link(ax, (5.45, y), (5.8, y), both=True)
    ax.text(4.55, 5.90, "以太网", fontsize=6.6, color="0.3")

    # The user computer hangs off the switch through the back-panel RJ45.
    verts = [(4.45, 3.55), (4.45, 1.30), (11.55, 1.30), (11.55, 4.65),
             (12.00, 4.65)]
    for (x0, y0), (x1, y1) in zip(verts, verts[1:]):
        ax.plot([x0, x1], [y0, y1], color="0.25", lw=1.0)
    ax.plot([12.00, 11.95], [4.65, 4.65], color="0.25", lw=1.0)
    ax.add_patch(FancyArrowPatch((11.98, 4.65), (12.00, 4.65),
                                 arrowstyle="-|>", mutation_scale=8, lw=1.0,
                                 color="0.25"))
    ax.text(6.5, 1.38, "以太网（从背部网口引出）", fontsize=6.8, color="0.3")

    link(ax, (9.10, 6.40), (9.70, 6.80), "USB2.0", tx=0.0, ty=0.28)
    link(ax, (11.25, 6.90), (12.00, 6.90), "无线", tx=0.0, ty=0.14)


def draw_ports(ax) -> None:
    ax.set_xlim(0, 13)
    ax.set_ylim(0, 7.9)
    ax.axis("off")
    ax.set_title("背部接口示意：A1（上）与 Go1（下）", fontsize=10)

    a1_ports = [
        "① HDMI", "② USB3.0", "③ USB2.0", "④ RJ45 以太网", "⑤ 24V 输入",
        "⑥ 24V 输入", "⑦ 底层串口（不开放）", "⑧ 底层串口（不开放）",
        "⑨ 5V/2A 输出", "⑩ 12V/2A 输出", "⑪ 19V/2A 输出", "⑫ RJ45 以太网",
        "⑬ UP Board USB2.0", "⑭ UP Board USB3.0", "⑮ UP Board HDMI",
    ]
    go1_ports = [
        "① RJ45 以太网", "② 树莓派 USB", "③ 树莓派 HDMI", "④ Nano3 USB",
        "⑤ Nano3 HDMI", "⑥ Nano2 USB", "⑦ Nano2 HDMI", "⑧ 外部拓展接口",
        "⑨ Type-C（不开放）", "⑩ Type-C（不开放）", "⑪ 24V 输入",
    ]

    def panel(y, name, count, ports):
        ax.add_patch(Rectangle((0.3, y), 12.4, 3.15, facecolor="#fafafa",
                               edgecolor="0.35", lw=1.0))
        ax.text(0.5, y + 2.85, name, fontsize=9.5, weight="bold")
        cols = 5
        for i in range(count):
            r, c = divmod(i, cols)
            cx = 1.0 + c * 2.35
            cy = y + 2.35 - r * 0.40
            ax.add_patch(Rectangle((cx - 0.16, cy - 0.13), 0.32, 0.26,
                                   facecolor="#cfd8dc", edgecolor="0.25"))
            ax.text(cx, cy, str(i + 1), ha="center", va="center", fontsize=6.5)
        for i, text in enumerate(ports):
            c = i % 4
            r = i // 4
            ax.text(0.5 + c * 3.12, y + 1.18 - r * 0.25, text, fontsize=6.1,
                    color="0.2")

    panel(4.35, "A1（15 个接口；④ 与 ⑫ 等价，都是交换机网口）", 15, a1_ports)
    panel(0.60, "Go1（11 个接口；⑨ ⑩ 为不开放的 Type-C）", 11, go1_ports)
    ax.text(0.3, 0.18, "线稿只表示相对位置与编号，不对应真实针脚间距；插拔前先断电。",
            fontsize=7.2, color="0.35")


def terminal(ax, lines, title=None, fs=7.2, color="#e6e6e6"):
    ax.set_xlim(0, 1)
    ax.set_ylim(0, 1)
    ax.axis("off")
    if title:
        ax.set_title(title, fontsize=9)
    ax.add_patch(Rectangle((0, 0), 1, 1, facecolor="#1e1e1e", edgecolor="0.2"))
    dy = 0.9 / max(len(lines), 1)
    for i, line in enumerate(lines):
        ax.text(0.025, 0.92 - i * dy, line, family=MONO, fontsize=fs,
                color=color, va="top")


def draw_ip_addr(ax) -> None:
    lines = [
        "$ ip addr show enp3s0",
        "2: enp3s0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500",
        "    link/ether 00:0e:c6:61:29:21 brd ff:ff:ff:ff:ff:ff",
        "    inet 192.168.123.162/24 brd 192.168.123.255 scope global enp3s0",
        "       valid_lft forever preferred_lft forever",
    ]
    terminal(ax, lines, "ip addr：网口名、inet 地址与 /24 前缀", fs=8.2)


def draw_ping_ssh_scp(ax) -> None:
    lines = [
        "$ ping -c 3 192.168.123.161",
        "64 bytes from 192.168.123.161: icmp_seq=1 ttl=64 time=0.83 ms",
        "64 bytes from 192.168.123.161: icmp_seq=2 ttl=64 time=0.79 ms",
        "$ ssh unitree@192.168.123.161",
        "unitree@192.168.123.161's password:",
        "unitree@a1-upboard:~$ hostname",
        "a1-upboard",
        "$ scp stand.py unitree@192.168.123.161:/home/unitree",
        "stand.py                    100% 4096     4.0KB/s   00:01",
    ]
    terminal(ax, lines, "ping 测连通、ssh 登录、scp 传文件（示意）", fs=7.4)


def draw_topology(ax) -> None:
    ax.set_xlim(0, 15.0)
    ax.set_ylim(0, 8.0)
    ax.axis("off")
    ax.set_title("机器人与路由器：LAN 侧固定 123 网段，WAN 侧接上一层网络", fontsize=10)
    ax.add_patch(Rectangle((0.2, 0.5), 6.8, 6.8, fill=False, ls="--",
                           edgecolor="0.5"))
    ax.text(0.45, 6.95, "机器人内部硬件", fontsize=8, color="0.4")

    box(ax, 1.0, 3.9, 2.2, 0.9, "交换机", "#eceff1")
    box(ax, 4.2, 3.7, 2.5, 1.3, "机器人计算机 1..n\n192.168.123.xxx",
        "#e3f2fd", fs=7.6)
    box(ax, 8.9, 0.75, 2.2, 0.95, "用户计算机\n192.168.123.xxx", "#fffde7",
        fs=7.6)
    box(ax, 8.9, 3.5, 2.6, 2.1,
        "路由器\nLAN 192.168.123.1/24\nWAN 自动获取", "#e8f5e9", fs=7.8)
    box(ax, 12.4, 4.0, 2.3, 1.3, "上一层网络\n实验室路由器 / modem",
        "#ede7f6", fs=7.4)

    link(ax, (3.4, 4.35), (4.2, 4.35), "以太网", tx=0.0, ty=0.18)
    verts = [(3.2, 3.9), (3.2, 2.8), (8.0, 2.8), (8.0, 4.2), (8.9, 4.2)]
    for (x0, y0), (x1, y1) in zip(verts, verts[1:]):
        ax.plot([x0, x1], [y0, y1], color="0.25", lw=1.0)
    ax.add_patch(FancyArrowPatch((8.85, 4.2), (8.9, 4.2), arrowstyle="-|>",
                                 mutation_scale=8, lw=1.0, color="0.25"))
    ax.text(3.3, 2.95, "以太网", fontsize=6.8, color="0.3")
    link(ax, (10.0, 0.75), (10.0, 3.5), "以太网", tx=0.55, ty=0.0)
    link(ax, (11.5, 4.6), (12.4, 4.6), "WAN", tx=0.0, ty=0.18)
    ax.text(1.3, 5.45, "LAN：192.168.123.0/24", fontsize=8, color="#1565c0")
    ax.text(7.35, 1.95, "默认网关 = 192.168.123.1", fontsize=8, color="#c62828")


def draw_route(ax) -> None:
    ax.set_xlim(0, 1)
    ax.set_ylim(0, 1)
    ax.axis("off")
    ax.set_title("ip route 与表 T03：两条 default 靠 metric 决定优先级", fontsize=9)
    ax.add_patch(Rectangle((0, 0.38), 1, 0.62, facecolor="#1e1e1e",
                           edgecolor="0.2"))
    lines = [
        "$ ip route",
        "default via 192.168.123.1 dev eth0 proto static metric 202",
        "default via 192.168.12.1 dev wlan1 proto static metric 304",
        "192.168.12.0/24 dev wlan1 proto kernel scope link src 192.168.12.161",
        "192.168.123.0/24 dev eth0 proto kernel scope link src 192.168.123.161",
    ]
    for i, line in enumerate(lines):
        ax.text(0.02, 0.94 - i * 0.09, line, family=MONO, fontsize=6.6,
                color="#e6e6e6", va="top")

    rows = [
        ("default", "192.168.123.1", "0.0.0.0", "UG", "202", "0", "0", "eth0"),
        ("default", "192.168.12.1", "0.0.0.0", "UG", "304", "0", "0", "wlan1"),
        ("192.168.12.0", "0.0.0.0", "255.255.255.0", "U", "304", "0", "0", "wlan1"),
        ("192.168.123.0", "0.0.0.0", "255.255.255.0", "U", "202", "0", "0", "eth0"),
        ("224.0.0.0", "0.0.0.0", "240.0.0.0", "U", "0", "0", "0", "lo"),
    ]
    header = ["Destination", "Gateway", "Genmask", "Flags", "Metric", "Ref",
              "Use", "Iface"]
    xs = [0.00, 0.20, 0.37, 0.55, 0.65, 0.74, 0.81, 0.89]
    ax.add_patch(Rectangle((0, 0.0), 1, 0.34, facecolor="white",
                           edgecolor="0.7"))
    for x, text in zip(xs, header):
        ax.text(x, 0.29, text, fontsize=5.6, weight="bold")
    for r, row in enumerate(rows):
        for x, text in zip(xs, row):
            ax.text(x, 0.235 - r * 0.045, text, fontsize=5.3, family=MONO)


def main() -> None:
    host = ipaddress.ip_interface(HOST_IP)
    subnet = ipaddress.ip_network("192.168.123.0/24")
    robot = ipaddress.ip_address(ROBOT_IP)
    print(f"输入: 网口 {INTERFACE}, 本机 {HOST_IP}, 机器人 {ROBOT_IP}")
    inside = host.ip in subnet
    duplicate = host.ip == robot or str(host.ip) in RESERVED
    usable = (inside and not duplicate
              and host.ip not in (subnet.network_address, subnet.broadcast_address))
    print(f"地址检查: {host.ip} 在 192.168.123.0/24 内: {'是' if inside else '否'}")
    print(f"与机器人已有地址冲突: {'是' if duplicate else '否'}")
    print(f"可以用于用户计算机: {'是' if usable else '否'}")
    print("已占用: " + ", ".join(f"{ip}({role})" for ip, role in RESERVED.items()))

    yaml = f"""network:
  version: 2
  ethernets:
    {INTERFACE}:
      dhcp4: false
      addresses: [{host.ip}/24]
      routes:
        - to: default
          via: {ROUTER_IP}
      nameservers:
        addresses: [192.168.123.1, 114.114.114.114]
"""
    print("netplan 片段（写入 /etc/netplan/60-robot.yaml 后执行 netplan apply）:")
    print(yaml, end="")

    print("ip route 期望输出（示意，实际以本机为准）:")
    print(f"default via {ROUTER_IP} dev {INTERFACE} proto static")
    print(f"192.168.123.0/24 dev {INTERFACE} proto kernel scope link "
          f"src {host.ip}")
    print("检查清单: ping -> ssh -> scp -> 启动控制器")
    print(f"  1. ping -c 3 {ROBOT_IP}      # 有回包、time 为毫秒级")
    print(f"  2. ssh unitree@{ROBOT_IP}     # 首次登录输入密码，exit 退出")
    print(f"  3. scp stand.py unitree@{ROBOT_IP}:/home/unitree")
    print("  4. 在机器人上启动控制器；先阻尼，再固定站立")
    print("本脚本不修改任何网络配置。")

    fig = plt.figure(figsize=(13.0, 7.0))
    gs = fig.add_gridspec(1, 1)
    draw_a1_network(fig.add_subplot(gs[0, 0]))
    savefig(fig, "3_6_a1_net")
    print("图已保存: figs/3_6_a1_net.png")

    fig = plt.figure(figsize=(14.0, 7.0))
    draw_go1_network(fig.add_subplot(111))
    savefig(fig, "3_6_go1_net")
    print("图已保存: figs/3_6_go1_net.png")

    fig = plt.figure(figsize=(12.6, 7.6))
    draw_ports(fig.add_subplot(111))
    savefig(fig, "3_6_ports")
    print("图已保存: figs/3_6_ports.png")

    fig = plt.figure(figsize=(9.0, 3.6))
    draw_ip_addr(fig.add_subplot(111))
    savefig(fig, "3_6_ipaddr")
    print("图已保存: figs/3_6_ipaddr.png")

    fig = plt.figure(figsize=(9.6, 4.8))
    draw_ping_ssh_scp(fig.add_subplot(111))
    savefig(fig, "3_6_ssh")
    print("图已保存: figs/3_6_ssh.png")

    fig = plt.figure(figsize=(13.0, 6.6))
    draw_topology(fig.add_subplot(111))
    savefig(fig, "3_6_topology")
    print("图已保存: figs/3_6_topology.png")

    fig = plt.figure(figsize=(10.5, 6.0))
    draw_route(fig.add_subplot(111))
    savefig(fig, "3_6_route")
    print("图已保存: figs/3_6_route.png")


if __name__ == "__main__":
    main()
