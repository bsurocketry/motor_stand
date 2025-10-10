gcc -o build/load_cell_serv.e -pthread src/load_cell_serv.c src/numerical_basics.c src/method_of_least_squares.c -lpigpio -lrt
gcc -o build/load_cell_logger.e -pthread src/load_cell_cli.c src/load_cell_logger.c
gcc -o build/load_cell_actor.e -pthread src/load_cell_cli.c src/load_cell_actor.c
gcc -o build/load_cell_visual.e src/teststandui.c src/load_cell_cli.c $(pkg-config --cflags --libs gtk+-3.0) -lm
gcc -o build/output_rewrite.e src/output_rewrite.c

#!/bin/sh

# Cross-platform build helper.
# If CMake is available, use it. Otherwise, attempt a best-effort gcc build.

set -e

if command -v cmake >/dev/null 2>&1; then
	echo "CMake found, building with CMake"
	mkdir -p build
	cd build
	cmake ..
	cmake --build . --config Release
	echo "Build complete in ./build"
	exit 0
fi

echo "CMake not found, falling back to legacy gcc commands"

# Detect platform to conditionally enable pigpio/rt linking
UNAME=$(uname -s 2>/dev/null || echo "")
IS_WINDOWS=0
IS_LINUX=0
case "$UNAME" in
	Linux*) IS_LINUX=1 ;;
	MINGW*|MSYS*|CYGWIN*) IS_WINDOWS=1 ;;
esac

mkdir -p build

if [ "$IS_LINUX" -eq 1 ]; then
	echo "Detected Linux: enabling pigpio and librt"
	gcc -o build/load_cell_serv.e -pthread src/load_cell_serv.c src/numerical_basics.c src/method_of_least_squares.c -lpigpio -lrt || true
	gcc -o build/load_cell_logger.e -pthread src/load_cell_cli.c src/load_cell_logger.c || true
	gcc -o build/load_cell_actor.e -pthread src/load_cell_cli.c src/load_cell_actor.c || true
else
	echo "Non-Linux environment detected: skipping server/logger/actor builds that require POSIX sockets."
fi

# build GUI app: try pkg-config for GTK; on Windows/MSYS2 users should install GTK packages
if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists gtk+-3.0; then
	GTK_CFLAGS=$(pkg-config --cflags gtk+-3.0)
	GTK_LIBS=$(pkg-config --libs gtk+-3.0)
	gcc -o build/load_cell_visual.e src/teststandui.c src/load_cell_cli.c $GTK_CFLAGS $GTK_LIBS -lm || true
else
	echo "pkg-config or gtk+-3.0 not available; skipping build of load_cell_visual (GTK app)"
fi

gcc -o build/output_rewrite.e src/output_rewrite.c || true

echo "Legacy build finished (some targets may have been skipped)."
