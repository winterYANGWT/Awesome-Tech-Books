# needs: mpi
# timeout: 120
# output: varies
# expect: best_cost=338[\s\S]*energy_total=4.000000
set -euo pipefail
: "${BUILD:?}" "${CXX:?}" "${CXXFLAGS:?}" "${MPIRUN:?}"
mkdir -p "$BUILD"

mpicxx $CXXFLAGS tsp_dynamic.cpp -o "$BUILD/tsp_dynamic"
$MPIRUN -np 4 "$BUILD/tsp_dynamic"
