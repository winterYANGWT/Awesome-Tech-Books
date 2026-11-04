// 把一个带填充的结构体当作一个元素广播：派生类型一次发，和拆成三次 Bcast 对照。
#include <mpi.h>

#include <cstddef>
#include <cstdio>

// ANCHOR: core
struct Particle {
    double x;
    double y;
    int id;
};

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    Particle p{};
    if (rank == 0) {
        p = Particle{1.5, -2.25, 7};
    }

    MPI_Datatype particle_type = MPI_DATATYPE_NULL;
    {
        int blocks[3] = {1, 1, 1};
        MPI_Aint base = 0;
        MPI_Aint disp[3] = {0, 0, 0};
        MPI_Datatype types[3] = {MPI_DOUBLE, MPI_DOUBLE, MPI_INT};
        MPI_Get_address(&p, &base);
        MPI_Get_address(&p.x, &disp[0]);
        MPI_Get_address(&p.y, &disp[1]);
        MPI_Get_address(&p.id, &disp[2]);
        for (int i = 0; i < 3; ++i) {
            disp[i] -= base;
        }
        MPI_Type_create_struct(3, blocks, disp, types, &particle_type);
        MPI_Type_commit(&particle_type);
    }

    MPI_Bcast(&p, 1, particle_type, 0, MPI_COMM_WORLD);

    // 对照：同样的字段拆成三次独立的 Bcast。
    Particle q{};
    if (rank == 0) {
        q = p;
    }
    MPI_Bcast(&q.x, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&q.y, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&q.id, 1, MPI_INT, 0, MPI_COMM_WORLD);

    double xs[8] = {};
    double ys[8] = {};
    double qx[8] = {};
    double qy[8] = {};
    int ids[8] = {};
    int qids[8] = {};
    MPI_Gather(&p.x, 1, MPI_DOUBLE, xs, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Gather(&p.y, 1, MPI_DOUBLE, ys, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Gather(&p.id, 1, MPI_INT, ids, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Gather(&q.x, 1, MPI_DOUBLE, qx, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Gather(&q.y, 1, MPI_DOUBLE, qy, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Gather(&q.id, 1, MPI_INT, qids, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        bool same = true;
        std::printf("派生类型一次 Bcast:\n");
        for (int i = 0; i < size; ++i) {
            std::printf("  rank %d: x=%.2f y=%.2f id=%d\n", i,
                        xs[i], ys[i], ids[i]);
            if (xs[i] != qx[i] || ys[i] != qy[i] || ids[i] != qids[i]) {
                same = false;
            }
        }
        std::printf("三次 Bcast 对照:\n");
        for (int i = 0; i < size; ++i) {
            std::printf("  rank %d: x=%.2f y=%.2f id=%d\n", i,
                        qx[i], qy[i], qids[i]);
        }
        std::printf("两次结果一致: %s\n", same ? "true" : "false");
    }

    MPI_Type_free(&particle_type);
    MPI_Finalize();
    return 0;
}
// ANCHOR_END: core
