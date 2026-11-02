"""Compile production input code against fake hardware, then run on the PC."""
import pathlib
import shutil
import subprocess
import tempfile

tests = pathlib.Path(__file__).resolve().parent
controller = tests.parent
compiler = shutil.which("gcc")
if compiler is None:
    raise SystemExit("Host GCC is required (not arm-none-eabi-gcc).")
with tempfile.TemporaryDirectory(prefix="rdc-controller-tests-") as build:
    executable = pathlib.Path(build) / "controller-tests.exe"
    subprocess.run([
        compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
        f"-I{tests / 'stubs'}", f"-I{controller / 'Core/Inc'}",
        str(tests / "test_controller.c"),
        str(controller / "Core/Src/buttons.c"),
        str(controller / "Core/Src/joystick.c"), "-lm",
        str(controller / "Core/Src/controller_link.c"),
        "-o", str(executable),
    ], check=True)
    subprocess.run([str(executable)], check=True)
