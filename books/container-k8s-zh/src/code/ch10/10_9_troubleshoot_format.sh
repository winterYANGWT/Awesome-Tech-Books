# 10.7 的第二个故障：镜像里有 arm64，但这台机器跑不了 arm64
uname -m
echo "registered binfmt handlers: $(ls /proc/sys/fs/binfmt_misc 2>/dev/null | wc -l)"

#> fails
#> expect: exec format error
docker run --rm --platform linux/arm64 localhost:5000/shorty:0.1 version

# 同一张 manifest list，换成 amd64 就正常
docker run --rm --platform linux/amd64 localhost:5000/shorty:0.1 version
