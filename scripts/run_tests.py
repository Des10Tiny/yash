import os
import subprocess
import sys
from pathlib import Path


class Colors:
    WARNING = "\033[93m"
    FAIL = "\033[91m"
    HEADER = "\033[95m"
    GREEN = "\033[92m"
    ENDC = "\033[0m"


def color_print(text: str, color: str) -> None:
    print(f"{color}{text}{Colors.ENDC}")


def is_executable(path: Path) -> bool:
    return (
        path.is_file()
        and os.access(path, os.X_OK)
        and path.name.startswith("yashTests")
        and not path.name.endswith(".cmake")
    )


def is_sanitizer(path: Path) -> bool:
    name = path.name.lower()
    return any(keyword in name for keyword in ("sanitiz", "asan", "ubsan", "tsan"))


def run_single_binary(binary: Path) -> int:
    color_print(f"--- Starting: {binary} ---", Colors.WARNING)

    process = subprocess.run([str(binary.resolve())])

    if process.returncode != 0:
        color_print(
            f"!!! Test binary {binary} FAILED with code {process.returncode} !!!",
            Colors.FAIL,
        )
    else:
        color_print(f"--- Finished: {binary} (SUCCESS) ---", Colors.GREEN)

    return process.returncode


def run_all_tests() -> None:
    search_path = Path(sys.argv[1] if len(sys.argv) > 1 else "./tests")

    if not search_path.exists():
        color_print(
            f"No executable test binaries found in {search_path}!", Colors.HEADER
        )
        sys.exit(0)

    test_binaries: list[Path] = [
        path for path in search_path.rglob("yashTests*") if is_executable(path)
    ]

    if not test_binaries:
        color_print(
            f"No executable test binaries found in {search_path}!", Colors.HEADER
        )
        sys.exit(0)

    test_binaries.sort(key=lambda p: (1 if is_sanitizer(p) else 0, p.name, str(p)))

    color_print(
        f"Found {len(test_binaries)} test binaries: {[str(p) for p in test_binaries]}",
        Colors.HEADER,
    )

    failed_binaries: list[Path] = []

    for binary in test_binaries:
        if is_sanitizer(binary) and failed_binaries:
            color_print(
                "\nSkipping sanitized tests because earlier tests failed.",
                Colors.WARNING,
            )
            break

        return_code = run_single_binary(binary)
        if return_code != 0:
            failed_binaries.append(binary)

    if failed_binaries:
        color_print("\nSome tests failed!", Colors.FAIL)
        sys.exit(1)

    color_print("\nAll tests passed successfully!", Colors.GREEN)
    sys.exit(0)


if __name__ == "__main__":
    run_all_tests()
