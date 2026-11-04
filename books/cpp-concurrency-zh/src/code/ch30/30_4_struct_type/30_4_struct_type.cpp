// 用 MPI_Type_create_struct 描述 {double,double,int}，打印 sizeof、偏移、size 和 extent。
#include <mpi.h>

#include <cstddef>
#include <cstdio>
#include <vector>

// ANCHOR: core
struct Record {
    double a;
    double b;
    int id;
};

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    Record r{};
    if (rank == 0) {
        r = Record{1.5, -2.5, 7};
    }

    int blocks[3] = {1, 1, 1};
    MPI_Aint base = 0;
    MPI_Aint disp[3] = {0, 0, 0};
    MPI_Datatype types[3] = {MPI_DOUBLE, MPI_DOUBLE, MPI_INT};
    MPI_Get_address(&r, &base);
    MPI_Get_address(&r.a, &disp[0]);
    MPI_Get_address(&r.b, &disp[1]);
    MPI_Get_address(&r.id, &disp[2]);
    for (int i = 0; i < 3; ++i) {
        disp[i] -= base;
    }

    MPI_Datatype record_type = MPI_DATATYPE_NULL;
    MPI_Type_create_struct(3, blocks, disp, types, &record_type);

    MPI_Aint lb = 0;
    MPI_Aint extent = 0;
    int type_size = 0;
    MPI_Type_get_extent(record_type, &lb, &extent);
    MPI_Type_size(record_type, &type_size);

    // 结构体末尾有填充；要让“Record 数组”连续发送，extent 必须等于 sizeof。
    MPI_Datatype array_type = MPI_DATATYPE_NULL;
    MPI_Type_create_resized(record_type, 0, static_cast<MPI_Aint>(sizeof(Record)),
                            &array_type);
    MPI_Type_commit(&array_type);

    MPI_Aint lb2 = 0;
    MPI_Aint extent2 = 0;
    MPI_Type_get_extent(array_type, &lb2, &extent2);

    MPI_Bcast(&r, 1, array_type, 0, MPI_COMM_WORLD);

    std::vector<double> as(static_cast<std::size_t>(size), 0.0);
    std::vector<double> bs(static_cast<std::size_t>(size), 0.0);
    std::vector<int> ids(static_cast<std::size_t>(size), 0);
    MPI_Gather(&r.a, 1, MPI_DOUBLE, as.data(), 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Gather(&r.b, 1, MPI_DOUBLE, bs.data(), 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Gather(&r.id, 1, MPI_INT, ids.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::printf("sizeof(Record)=%zu\n", sizeof(Record));
        std::printf("offsetof: a=%zu b=%zu id=%zu\n", offsetof(Record, a),
                    offsetof(Record, b), offsetof(Record, id));
        std::printf("MPI_Get_address 偏移: a=%ld b=%ld id=%ld\n",
                    static_cast<long>(disp[0]), static_cast<long>(disp[1]),
                    static_cast<long>(disp[2]));
        std::printf("MPI_Type_size=%d\n", type_size);
        std::printf("create_struct extent=%ld\n", static_cast<long>(extent));
        std::printf("resized extent=%ld\n", static_cast<long>(extent2));
        for (int i = 0; i < size; ++i) {
            std::printf("rank %d 收到: a=%.2f b=%.2f id=%d\n", i,
                        as[static_cast<std::size_t>(i)],
                        bs[static_cast<std::size_t>(i)],
                        ids[static_cast<std::size_t>(i)]);
        }
    }

    MPI_Type_free(&array_type);
    MPI_Type_free(&record_type);
    MPI_Finalize();
    return 0;
}
// ANCHOR_END: core
