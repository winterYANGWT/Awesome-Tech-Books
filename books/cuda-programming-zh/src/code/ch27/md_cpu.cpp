// md_cpu.cpp: 分子动力学的 CPU 基线版本，统计每个阶段的耗时，并检查能量守恒。
//   nvcc -O2 md_cpu.cpp -o md_cpu && ./md_cpu [每个方向的晶胞数] [步数]
#include "md_common.h"
#include <cstdlib>

int main(int argc, char** argv) {
    Params p;
    if (argc > 1) p.nc = atoi(argv[1]);
    if (argc > 2) p.steps = atoi(argv[2]);
    System s;
    init_system(s, p);
    s.max_nbr = 128;
    const double rlist = p.rc + p.skin;
    printf("%d 个粒子，盒子边长 %.2f，%d 步\n", s.n, s.box, p.steps);

    Timer t_nbr, t_force, t_int;
    t_nbr.start();
    build_neighbors(s, rlist, true);
    t_nbr.stop();
    t_force.start();
    forces_cpu(s, p.rc);
    t_force.stop();
    double e0 = kinetic(s) + potential(s);
    printf("  步数     动能/N     势能/N     总能量/N\n");
    printf("%6d  %9.5f  %9.5f  %10.6f\n", 0, kinetic(s) / s.n, potential(s) / s.n, e0 / s.n);

    // ANCHOR: loop
    for (int step = 1; step <= p.steps; ++step) {
        t_int.start();
        kick(s, 0.5 * p.dt);                  // 半步速度
        drift(s, p.dt);                       // 整步位置
        t_int.stop();
        if (step % p.rebuild_every == 0) {    // 粒子走动了，定期重建邻居表
            t_nbr.start();
            build_neighbors(s, rlist, true);
            t_nbr.stop();
        }
        t_force.start();
        forces_cpu(s, p.rc);                  // 新位置上的力
        t_force.stop();
        t_int.start();
        kick(s, 0.5 * p.dt);                  // 另外半步速度
        t_int.stop();
        if (step % p.report_every == 0) {
            double ke = kinetic(s), pe = potential(s);
            printf("%6d  %9.5f  %9.5f  %10.6f\n", step, ke / s.n, pe / s.n, (ke + pe) / s.n);
        }
    }
    // ANCHOR_END: loop
    double e1 = kinetic(s) + potential(s);
    double total = t_nbr.total + t_force.total + t_int.total;
    int max_count = 0;
    for (int c : s.nbr_count) max_count = c > max_count ? c : max_count;
    printf("总能量相对漂移 %.1e，邻居表里最多 %d 个邻居（上限 %d）\n", std::fabs(e1 - e0) / std::fabs(e0), max_count,
           s.max_nbr);
    printf("\n阶段        时间        占比    每步\n");
    printf("求力    %9.1f ms  %5.1f%%  %6.2f ms\n", t_force.total, 100 * t_force.total / total, t_force.total / p.steps);
    printf("邻居表  %9.1f ms  %5.1f%%  %6.2f ms\n", t_nbr.total, 100 * t_nbr.total / total, t_nbr.total / p.steps);
    printf("积分    %9.1f ms  %5.1f%%  %6.2f ms\n", t_int.total, 100 * t_int.total / total, t_int.total / p.steps);
    printf("合计    %9.1f ms          %6.2f ms\n", total, total / p.steps);
}
