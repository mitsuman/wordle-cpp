set -e

OUT=out/release
cmake -S . -B $OUT
cmake --build $OUT
${OUT}/wordle --solver entropy --use-hard
