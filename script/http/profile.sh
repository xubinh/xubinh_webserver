#!/usr/bin/env bash

APP_NAME="http"
SERVER_EXECUTABLE_PATH="./build/example/$APP_NAME/server/$APP_NAME""_server"
SLEEP_SECONDS=1

BUILD_SCRIPT_PATH="./script/build.sh"
BUILD_COMMAND="CMAKE_BUILD_TYPE=Debug ENABLE_TEST=off RUN_BENCHMARK=on $BUILD_SCRIPT_PATH"

WEBBENCH_EXECUTABLE_PATH="./bin/webbench"
NUMBER_OF_SECONDS_TO_TEST=60

PERF_DATA_FILE_PATH="perf.data"
PERF_SCRIPT_OUTPUT_FILE_PATH="$PERF_DATA_FILE_PATH".out
FOLDED_PERF_SCRIPT_OUTPUT_FILE_PATH="$PERF_SCRIPT_OUTPUT_FILE_PATH".folded
FLAME_GRAPH_SVG_OUTPUT_FILE_PATH='flamegraph.svg'

clear

echo "Building..."

if ! eval "$BUILD_COMMAND"; then
    echo "Building failed ❌"

    exit 1
fi

echo "Elevating permission for perf..."

sudo sysctl kernel.perf_event_paranoid=1
sudo sysctl kernel.kptr_restrict=0

"$SERVER_EXECUTABLE_PATH" &

server_pid="$!"

echo "Server started, PID: $server_pid"

echo "Sleep for $SLEEP_SECONDS second(s) for the server to be fully initialized..."

sleep "$SLEEP_SECONDS"

echo "Resumed"

perf record -F 99 -g --pid "$server_pid" >/dev/null 2>&1 &

perf_pid="$!"

echo "Perf started, PID: $perf_pid"

echo "Running WebBench, duration: $NUMBER_OF_SECONDS_TO_TEST second(s)..."

"$WEBBENCH_EXECUTABLE_PATH" -t "$NUMBER_OF_SECONDS_TO_TEST" -c 1000 -2 --get http://127.0.0.1:8080/ >/dev/null 2>&1

echo "WebBench finished, killing server..."

kill -INT "$server_pid"

echo "Server killed, waiting for perf to stop..."

while true; do
    sleep 1

    if ! ps -p "$perf_pid" >/dev/null 2>&1; then
        break
    fi
done

echo "Perf stopped, all info is collected and written to \`$PERF_DATA_FILE_PATH\`"

echo "Processing... (\`$PERF_DATA_FILE_PATH\` -> \`$PERF_SCRIPT_OUTPUT_FILE_PATH\`)"

perf script >"$PERF_SCRIPT_OUTPUT_FILE_PATH"

echo "Folding... (\`$PERF_SCRIPT_OUTPUT_FILE_PATH\` -> \`$FOLDED_PERF_SCRIPT_OUTPUT_FILE_PATH\`)"

../FlameGraph/stackcollapse-perf.pl "$PERF_SCRIPT_OUTPUT_FILE_PATH" >"$FOLDED_PERF_SCRIPT_OUTPUT_FILE_PATH"

echo "Drawing... (\`$FOLDED_PERF_SCRIPT_OUTPUT_FILE_PATH\` -> \`$FLAME_GRAPH_SVG_OUTPUT_FILE_PATH\`)"

# gray color is not supported by default; one could go to the flamegraph code base and write his own color palette
# ../FlameGraph/flamegraph.pl "$FOLDED_PERF_SCRIPT_OUTPUT_FILE_PATH" >"$FLAME_GRAPH_SVG_OUTPUT_FILE_PATH"
../FlameGraph/flamegraph.pl --colors=gray --fonttype="monospace" --height=20 \
    "$FOLDED_PERF_SCRIPT_OUTPUT_FILE_PATH" >"$FLAME_GRAPH_SVG_OUTPUT_FILE_PATH"

echo "Removing auxiliary files..."

rm "$PERF_DATA_FILE_PATH"* # perf.data, perf.data.out, perf.data.out.folded

echo "Completed, result is at \`$FLAME_GRAPH_SVG_OUTPUT_FILE_PATH\`. ✔️"
