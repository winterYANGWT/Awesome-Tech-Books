# needs: mpi
# timeout: 30
# expect: 收齐 3.000
set -euo pipefail
: "${BUILD:?}" "${CXX:?}" "${CXXFLAGS:?}" "${MPIRUN:?}"
mkdir -p "$BUILD"

mpicxx $CXXFLAGS energy_trace.cpp -o "$BUILD/energy_trace"
$MPIRUN -np 3 "$BUILD/energy_trace"
