"""List USB serial ports and print a udev rule template (section 2.4).

The scan is read-only.  With no adapter attached the script prints a short
message and exits with status 0, so it is safe to run before buying hardware.

expect: 扫描 /dev/ttyUSB
expect: 未发现 USB 串口
expect: SUBSYSTEM=="tty"
"""

from __future__ import annotations

import glob
import grp
import os
import pwd
import stat

PATTERNS = ("/dev/ttyUSB*", "/dev/ttyACM*")
RULE = ('SUBSYSTEM=="tty", ATTRS{idVendor}=="<idVendor>", '
        'ATTRS{idProduct}=="<idProduct>", ATTRS{serial}=="<serial>", '
        'SYMLINK+="a1_motor"')


def describe(path: str) -> str:
    info = os.stat(path)
    mode = stat.filemode(info.st_mode)
    try:
        owner = pwd.getpwuid(info.st_uid).pw_name
    except KeyError:
        owner = str(info.st_uid)
    try:
        group = grp.getgrgid(info.st_gid).gr_name
    except KeyError:
        group = str(info.st_gid)
    return f"{path}  权限 {mode}  属主 {owner}:{group}"


def main() -> None:
    print("扫描 /dev/ttyUSB* 与 /dev/ttyACM* ...")
    devices: list[str] = []
    for pattern in PATTERNS:
        devices.extend(sorted(glob.glob(pattern)))
    devices = sorted(set(devices))

    if not devices:
        print("未发现 USB 串口")
        print("把 USB 转 RS-485 转接器插好、确认驱动加载后重新运行。")
    else:
        print(f"发现 {len(devices)} 个串口：")
        for path in devices:
            print("  " + describe(path))

    print("udev 规则模板（用 lsusb 和 udevadm info -a -n <设备> 替换尖括号里的值）：")
    print(RULE)
    print("权限：sudo usermod -aG dialout $USER，重新登录后生效；不要长期用 sudo 跑控制程序。")


if __name__ == "__main__":
    main()
