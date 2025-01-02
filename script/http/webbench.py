#!/usr/bin/env python3

import os
import re
import subprocess
import sys
import time
from os.path import join

ROOT: str = os.getcwd()
BUILD_DIR_NAME: str = "build"
APP_BASE_DIR_NAME: str = "example"
APP_DIR_NAME: str = "http"
SERVER_EXECUTABLE_NAME: str = APP_DIR_NAME + "_server"

BUILD_DIR_PATH: str = join(ROOT, BUILD_DIR_NAME)
APP_BASE_DIR_PATH: str = join(BUILD_DIR_PATH, APP_BASE_DIR_NAME)
APP_DIR_PATH: str = join(APP_BASE_DIR_PATH, APP_DIR_NAME)
APP_SERVER_DIR_PATH: str = join(APP_DIR_PATH, "server")
SERVER_EXECUTABLE_PATH: str = join(APP_SERVER_DIR_PATH, SERVER_EXECUTABLE_NAME)

SCRIPT_DIR_NAME: str = "script"
BUILD_SCRIPT_NAME: str = "build.sh"

SCRIPT_DIR_PATH: str = join(ROOT, SCRIPT_DIR_NAME)
BUILD_SCRIPT_PATH: str = join(SCRIPT_DIR_PATH, BUILD_SCRIPT_NAME)

BIN_DIR_NAME: str = "bin"
WEBBENCH_EXECUTABLE_NAME: str = "webbench"

BIN_DIR_PATH: str = join(ROOT, BIN_DIR_NAME)
WEBBENCH_EXECUTABLE_PATH: str = join(BIN_DIR_PATH, WEBBENCH_EXECUTABLE_NAME)

WEBBENCH_CLIENT_NUMBER: int = 1000
ENABLE_HTTP_1_1: bool = True
METHOD: str = "GET"
SERVER_IP: str = "127.0.0.1"
SERVER_PORT: str = "8080"

ENABLE_HTTP_1_1_SWITCH: str = "-2" if ENABLE_HTTP_1_1 else "-1"
METHOD_SWITCH: str = "--" + METHOD.lower()
SERVER_SOCKET_ADDRESS: str = f"{SERVER_IP}:{SERVER_PORT}"
SERVER_HTTP_URL: str = f"http://{SERVER_SOCKET_ADDRESS}/"

# for waiting the programs (e.g. server, webbench) to start/finish completely
SLEEP_SECONDS: int = 1

DEFAULT_TEST_DURATION: int = 2
DEFAULT_REPEAT_TIMES: int = 1


def clear() -> None:
    os.system("clear")


def exit(exit_code: int = 0) -> None:
    sys.exit(exit_code)


def run_build_script() -> int:
    command = BUILD_SCRIPT_PATH

    result = subprocess.run(command, shell=True)

    return result.returncode


def run_server(test_name: str) -> subprocess.Popen:
    server_process = subprocess.Popen(
        [SERVER_EXECUTABLE_PATH, test_name],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    return server_process


def run_webbench(duration: int) -> tuple[int, int]:
    command = "{} -t {} -c {} {} {} {}".format(
        WEBBENCH_EXECUTABLE_PATH,
        duration,
        WEBBENCH_CLIENT_NUMBER,
        ENABLE_HTTP_1_1_SWITCH,
        METHOD_SWITCH,
        SERVER_HTTP_URL,
    )

    result = subprocess.run(command, shell=True, capture_output=True, text=True)

    output = result.stdout

    try:
        succeed_match = re.search(r"(\d+)(?= succeed)", output)
        failed_match = re.search(r"(\d+)(?= failed)", output)

        assert succeed_match and failed_match

        succeed_number = int(succeed_match.group(1))
        failed_number = int(failed_match.group(1))

        return succeed_number, failed_number

    except:
        msg = "Output:\n" + output

        raise RuntimeError(msg)


def z_score_average(values: list[int], threshold: float = 3.0) -> int:
    assert len(values) > 0, "Empty list."

    mean = sum(values) / len(values)
    variance = sum((value - mean) ** 2 for value in values) / len(values)
    std_dev = variance**0.5

    # all values are identical
    if abs(std_dev) < 1e-8:
        return values[0]

    filtered_values = [
        value for value in values if abs((value - mean) / std_dev) <= threshold
    ]

    if not filtered_values:
        raise ValueError("No values left after removing outliers.")

    return int(sum(filtered_values) / len(filtered_values))


def get_input_arguments() -> tuple[int, int]:
    if len(sys.argv) > 3:
        print("Usage: script.py <duration_in_seconds> <number_of_repetition>")

        exit(1)

    try:
        test_duration = int(sys.argv[1]) if len(sys.argv) > 1 else 2
        repeat_times = int(sys.argv[2]) if len(sys.argv) > 2 else 1

    except ValueError:
        print("Usage: script.py <duration_in_seconds> <number_of_repetition>")

        exit(1)

    return test_duration, repeat_times


def main():
    test_duration, repeat_times = get_input_arguments()

    clear()

    print("Building...")

    return_code = run_build_script()

    if return_code != 0:
        print("Building failed ❌")

        exit(1)

    test_results: list[int] = []

    for i in range(repeat_times):
        print("--------------------------------")
        print(f"Iteration: {i + 1}")

        test_name = f"server-test-{i + 1}"

        server_process = run_server(test_name)

        print(f"Server started, pid: {server_process.pid}")

        print(
            f"Sleep for {SLEEP_SECONDS} second(s) for the server to be fully initialized..."
        )

        time.sleep(SLEEP_SECONDS)

        print("Resumed")

        print(f"Running WebBench, duration: {test_duration} second(s)...")

        succeed_number, failed_number = run_webbench(test_duration)

        print("WebBench finished, killing server...")

        server_process.terminate()
        server_process.wait()

        print(
            f"Sleep for {SLEEP_SECONDS} second(s) for the server to be fully shutdown..."
        )

        time.sleep(SLEEP_SECONDS)

        print("Resumed")

        print(f"Succeed number: {succeed_number}")
        print(f"Failed number: {failed_number}")

        if failed_number > 0:
            output_file_name = f"server-crash-report-{i + 1}.txt"

            print(
                f"Something went wrong, `stdout` and `stderr` are output to file: ./{output_file_name}"
            )
            assert server_process.stdout and server_process.stderr

            with open(output_file_name, "wb") as file:
                file.write(b"stdout:\n\n")
                file.write(server_process.stdout.read())
                file.write(b"\nstdout:\n\n")
                file.write(server_process.stderr.read())

        test_results.append(succeed_number)

    # average_qps: int = int(z_score_average(test_results) / test_duration)
    # print("\nAverage QPS:", average_qps)
    max_qps: int = int(max(test_results) / test_duration)
    print("\nMax QPS:", max_qps)


if __name__ == "__main__":
    main()
