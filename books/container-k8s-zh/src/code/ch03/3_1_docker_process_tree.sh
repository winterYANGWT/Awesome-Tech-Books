# 3.1 一次 docker run 之后，进程树里留下了谁
# 先让 shorty 在后台跑起来。setup.sh 已经构建了 shorty:0.1。
#> expect: ^[0-9a-f]{64}$
docker run -d --name ch03-shorty shorty:0.1 serve

# Docker 自己的视图：容器在运行，镜像是 shorty:0.1。
docker ps --filter name=ch03-shorty --format 'table {{.Names}}\t{{.Image}}\t{{.Status}}'

# 进程视图：只取 shorty 的 PID、它的父进程 shim、dockerd 和 PID 1。
# 不要用 grep shorty 过滤整张进程表，lab 里可能有别的实验进程也叫 shorty。
pid=$(docker inspect -f '{{.State.Pid}}' ch03-shorty)
ppid=$(ps -o ppid= -p "$pid" | tr -d ' ')
ps -o pid,ppid,user,args -p 1 -p "$(pgrep -x dockerd)" -p "$ppid" -p "$pid"

# 从 shim 开始画：shim 下面是 shorty。shim 自己的父进程是 1，ps 那行能对上。
pstree -p "$ppid" | sed -n '1,5p'

# dockerd 这一支单独看，它启动的线程很多，只取前几行。
pstree -p "$(pgrep -x dockerd)" | sed -n '1,8p'

# Docker 报告的 containerd 版本。lab 的 dind 没有单独安装 containerd，
# 这个进程被嵌在 dockerd 里，所以 ps 中看不到独立的 containerd。
docker version | sed -n '/containerd:/,/runc:/p'
docker info --format 'Server {{.ServerVersion}} | Storage {{.Driver}} | Cgroup {{.CgroupDriver}}'

# 换 containerd 自己的客户端 ctr，从它的命名空间里能看到同一个容器。
ctr --address /var/run/docker/containerd/containerd.sock --namespace moby containers list
ctr --address /var/run/docker/containerd/containerd.sock --namespace moby tasks list

# runc 的视角：容器还在 running，bundle 在 containerd 的 task 目录下。
runc --root /var/run/docker/runtime-runc/moby list

# 观察完就删掉，下一节手工做 bundle 时环境是干净的。
docker rm -f ch03-shorty
