# needs: mpi
# timeout: 120
# output: varies
# expect: np=4 checksum=
set -euo pipefail
: "${BUILD:?}" "${CXX:?}" "${CXXFLAGS:?}" "${MPIRUN:?}"
mkdir -p "$BUILD"

mpicxx $CXXFLAGS nbody_ring.cpp -o "$BUILD/nbody_ring"

for np in 1 2 4 8; do
    $MPIRUN -np "$np" "$BUILD/nbody_ring"
done
