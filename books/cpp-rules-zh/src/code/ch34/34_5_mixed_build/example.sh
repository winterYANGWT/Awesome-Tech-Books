set -e

$CXX -std=c++20 -Wall -Wextra -Werror --precompile pricing.cppm -o "$BUILD/pricing.pcm"

$CXX -std=c++20 -Wall -Wextra -Werror -c legacy_pricing.cpp -o "$BUILD/legacy_pricing.o"

$CXX -std=c++20 -Wall -Wextra -Werror -fmodule-file=pricing="$BUILD/pricing.pcm" \
    new_client.cpp "$BUILD/pricing.pcm" "$BUILD/legacy_pricing.o" -o "$BUILD/new_client"
"$BUILD/new_client"

$CXX -std=c++20 -Wall -Wextra -Werror \
    old_client.cpp legacy_pricing.cpp -o "$BUILD/old_client"
"$BUILD/old_client"
