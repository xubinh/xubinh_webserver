#!/usr/bin/env python3

import os
import re
import signal
import subprocess
import sys
import time

ROOT: str = os.getcwd()

# build script path
SCRIPT_DIR_PATH: str = f"{ROOT}/script"
BUILD_SCRIPT_PATH: str = f"{SCRIPT_DIR_PATH}/build.sh"

# server executable path
BUILD_DIR_PATH: str = f"{ROOT}/build"
APP_DIR_NAME: str = "http"
SERVER_EXECUTABLE_PATH: str = (
    f"{BUILD_DIR_PATH}/example/{APP_DIR_NAME}/server/{APP_DIR_NAME}_server"
)

# webbench executable path
BIN_DIR_PATH: str = f"{ROOT}/bin"
WEBBENCH_EXECUTABLE_PATH: str = f"{BIN_DIR_PATH}/webbench"

# for constructing webbench command
WEBBENCH_CLIENT_NUMBER: int = 1000
ENABLE_HTTP_1_1: bool = True
ENABLE_HTTP_1_1_SWITCH: str = "-2" if ENABLE_HTTP_1_1 else "-1"
METHOD_SWITCH: str = "--get"
SERVER_HTTP_URL: str = f"http://127.0.0.1:8080/"

# default testing config (i.e. one single 2-second test)
DEFAULT_TEST_DURATION: int = 2
DEFAULT_REPEAT_TIMES: int = 1

# for waiting the programs (e.g. server, webbench) to start/finish completely
SLEEP_SECONDS: int = 1


def clear() -> None:
    os.system("clear")


def exit(exit_code: int = 0) -> None:
    sys.exit(exit_code)


def run_build_script() -> int:
    command = BUILD_SCRIPT_PATH

    result = subprocess.run(command, shell=True)

    return result.returncode


def run_server(test_name: str, print_to_terminal: bool = False) -> subprocess.Popen:
    server_process = subprocess.Popen(
        [SERVER_EXECUTABLE_PATH, test_name],
        stdout=None if print_to_terminal else subprocess.PIPE,
        stderr=None if print_to_terminal else subprocess.PIPE,
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


def z_score_average(values: list[int], threshold: float = 2.0) -> tuple[int, int, int]:
    assert len(values) > 0, "empty list"

    mean = sum(values) / len(values)
    variance = sum((value - mean) ** 2 for value in values) / len(values)
    standard_deviation = variance**0.5

    # all values are identical
    if abs(standard_deviation) < 1e-8:
        return values[0], values[0], 0

    values = [
        value
        for value in values
        if abs((value - mean) / standard_deviation) <= threshold
    ]

    if not values:
        raise ValueError("no values left after removing outliers")

    mean = sum(values) / len(values)
    variance = sum((value - mean) ** 2 for value in values) / len(values)
    standard_deviation = variance**0.5

    return max(values), int(mean), int(standard_deviation)


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


def main(print_to_terminal: bool = False):
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

        server_process = run_server(test_name, print_to_terminal)

        print(f"Server started, pid: {server_process.pid}")

        print(
            f"Sleep for {SLEEP_SECONDS} second(s) for the server to be fully initialized..."
        )

        time.sleep(SLEEP_SECONDS)

        print("Resumed")

        print(f"Running WebBench, duration: {test_duration} second(s)...")

        succeed_number, failed_number = run_webbench(test_duration)

        print("WebBench finished, killing server...")

        server_process.send_signal(signal.SIGINT)
        server_process.wait()

        print(
            f"Sleep for {SLEEP_SECONDS} second(s) for the server to be fully shutdown..."
        )

        time.sleep(SLEEP_SECONDS)

        print("Resumed")

        print(f"Succeed number: {succeed_number}")
        print(f"Failed number: {failed_number}")

        if failed_number > 0 and not print_to_terminal:
            assert server_process.stdout and server_process.stderr

            stdout_data = server_process.stdout.read()
            stderr_data = server_process.stderr.read()

            if stdout_data or stderr_data:
                output_file_name = f"server-crash-report-{i + 1}.txt"

                print(
                    f"Something went wrong, `stdout` and `stderr` are output to file: ./{output_file_name}"
                )

                with open(output_file_name, "wb") as file:
                    file.write(b"stdout:\n\n")
                    file.write(stdout_data or b"<empty>")
                    file.write(b"\n\n")
                    file.write(b"stderr:\n\n")
                    file.write(stderr_data or b"<empty>")
                    file.write(b"\n")

        test_results.append(succeed_number)

    # queries per test
    max_qpt, mean_qpt, standard_deviation_qpt = z_score_average(test_results)

    max_qps: int = max_qpt // test_duration
    mean_qps: int = mean_qpt // test_duration
    standard_deviation_qps: int = standard_deviation_qpt // test_duration

    print("")
    print(f"Max QPS: {max_qps}")
    print(f"Average QPS: {mean_qps}")
    print(f"Standard Deviation: {standard_deviation_qps}")


if __name__ == "__main__":
    print_to_terminal = False
    # print_to_terminal = True
    main(print_to_terminal)
