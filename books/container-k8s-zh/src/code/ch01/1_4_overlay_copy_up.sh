# 手工搭一个 overlay：lower 只读，upper 可写，merged 是容器看到的根。
d=/tmp/ch01-ovl
if mountpoint -q "$d/merged"; then
    umount -l "$d/merged"
fi
rm -rf "$d"
mkdir -p "$d/lower" "$d/upper" "$d/work" "$d/merged"
echo "lower original" > "$d/lower/hello.txt"
mount -t overlay overlay \
    -o "lowerdir=$d/lower,upperdir=$d/upper,workdir=$d/work" "$d/merged"

echo '--- 第一次读：内容来自 lower ---'
cat "$d/merged/hello.txt"
echo '--- upper 里现在有什么 ---'
ls -A "$d/upper"

echo 'append from merged' >> "$d/merged/hello.txt"

echo '--- 写之后 upper 里多了副本 ---'
#> expect: hello.txt
ls -A "$d/upper"
echo '--- 副本内容 ---'
#> expect: append from merged
cat "$d/upper/hello.txt"
echo '--- lower 原文件没变 ---'
cat "$d/lower/hello.txt"
echo '--- 文件大小 ---'
stat -c '%s %n' "$d/lower/hello.txt" "$d/merged/hello.txt" "$d/upper/hello.txt"

echo '--- /proc/self/mountinfo 里的 overlay ---'
#> expect: lowerdir=
#> expect: upperdir=
#> expect: workdir=
grep " $d/merged " /proc/self/mountinfo | sed 's/,/,\n  /g'

umount "$d/merged"
rm -rf "$d"
