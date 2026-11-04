# needs: mpi
# timeout: 120
# output: varies
# expect: np=4 checksum=
set -euo pipefail
: "${BUILD:?}" "${CXX:?}" "${CXXFLAGS:?}" "${MPIRUN:?}"
mkdir -p "$BUILD"

mpicxx $CXXFLAGS nbody_allgather.cpp -o "$BUILD/nbody_allgather"

for np in 1 2 4 8; do
    $MPIRUN -np "$np" "$BUILD/nbody_allgather"
done
