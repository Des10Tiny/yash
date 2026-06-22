import subprocess
import sys
import os
import concurrent.futures


class Colors:
    WARNING = "\033[93m"
    FAIL = "\033[91m"
    HEADER = "\033[95m"
    GREEN = "\033[92m"
    ENDC = "\033[0m"


def color_print(text, color):
    print(f"{color}{text}{Colors.ENDC}")


def is_executable(path):
    return (
        os.path.isfile(path)
        and os.access(path, os.X_OK)
        and not path.endswith(".cmake")
    )


def run_single_binary(binary_path):
    color_print(f"--- Starting: {binary_path} ---", Colors.WARNING)

    process = subprocess.Popen(
        [os.path.abspath(binary_path)], stdout=sys.stdout, stderr=sys.stderr, text=True
    )

    return_code = process.wait()

    if return_code != 0:
        color_print(
            f"!!! Test binary {binary_path} FAILED with code {return_code} !!!",
            Colors.FAIL,
        )
    else:
        color_print(f"--- Finished: {binary_path} (SUCCESS) ---", Colors.GREEN)

    return return_code


def run_all_tests():
    search_path = "./tests"
    test_binaries = []

    if os.path.exists(search_path):
        for root, dirs, files in os.walk(search_path):
            for file in files:
                full_path = os.path.join(root, file)

                if file.startswith("yashTests") and is_executable(full_path):
                    test_binaries.append(full_path)

    if not test_binaries:
        color_print(
            f"No executable test binaries found in {search_path}!", Colors.HEADER
        )
        sys.exit(0)

    color_print(
        f"Found {len(test_binaries)} test binaries: {test_binaries}", Colors.HEADER
    )

    with concurrent.futures.ThreadPoolExecutor() as executor:
        results = list(executor.map(run_single_binary, test_binaries))

    if any(code != 0 for code in results):
        color_print("\nSome tests failed!", Colors.FAIL)
        sys.exit(1)

    color_print("\nAll tests passed successfully!", Colors.GREEN)
    sys.exit(0)


if __name__ == "__main__":
    run_all_tests()
