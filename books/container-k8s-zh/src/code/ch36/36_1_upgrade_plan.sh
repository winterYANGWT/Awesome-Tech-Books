# 36.1 先看版本偏差和升级计划。kind 节点里装的是 kubeadm 1.37.0，
# 计划升到同一 minor 的 v1.37.1；显式给版本，输出不随上游最新 patch 变化。
docker exec shorty-control-plane kubeadm version -o short

kubectl version

kubectl get nodes -o wide

#> expect: v1.37.1
#> expect: kubelet
#> expect: kube-apiserver
#> expect: etcd
# upgrade plan 会检查集群健康、版本偏差，并列出控制面和节点上要动的组件。
docker exec shorty-control-plane kubeadm upgrade plan v1.37.1
