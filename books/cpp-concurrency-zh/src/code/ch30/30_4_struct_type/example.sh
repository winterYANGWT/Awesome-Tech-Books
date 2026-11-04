#!/usr/bin/env bash
# needs: mpi
# expect: sizeof\(Record\)=24
# expect: offsetof: a=0 b=8 id=16
# expect: MPI_Type_size=20
# expect: create_struct extent=24
# expect: resized extent=24
set -euo pipefail

mpicxx $CXXFLAGS 30_4_struct_type.cpp -o "$BUILD/struct_type"
$MPIRUN -np 4 "$BUILD/struct_type"
