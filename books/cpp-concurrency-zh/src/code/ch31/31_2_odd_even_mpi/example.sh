# needs: mpi
# timeout: 30
# expect: 全局有序: 是
set -euo pipefail
: "${BUILD:?}" "${CXX:?}" "${CXXFLAGS:?}" "${MPIRUN:?}"
mkdir -p "$BUILD"

mpicxx $CXXFLAGS odd_even_mpi.cpp -o "$BUILD/odd_even_mpi"

$MPIRUN -np 4 "$BUILD/odd_even_mpi"
