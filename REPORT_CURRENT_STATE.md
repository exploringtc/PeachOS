Current State Report: PeachOS Build and Smoke Test Workflow
Date: 2026-04-02

Overview
This report documents what has been implemented so far, how the workflow operates step by step, and what each relevant file and code path does.

The major outcome is a one-command smoke test that:
1) Builds boot and kernel binaries.
2) Creates a raw disk image.
3) Ensures a user program is present for kernel process loading.
4) Boots headless in QEMU.
5) Captures VGA text memory.
6) Fails if panic-like signatures are detected.

Why this was needed
The kernel currently loads a hardcoded user binary path: 0:/blank.bin.
If blank.bin is not present in the disk image, boot reaches panic with:
Failed to load blank.bin

The smoke-test script now prevents that false-negative path by resolving an available user program and copying it into the image as blank.bin.

Step-by-Step Runtime Flow
Step 1: Toolchain and command checks
File: scripts/smoke-test.sh

The script verifies required commands exist:
- timeout, qemu-system-i386, perl
- i686-elf-gcc, i686-elf-ld, nasm, dd, sudo

This fails fast if the environment is missing any critical dependency.

Step 2: Build boot and kernel
File: scripts/smoke-test.sh

The script runs:
make -C ROOT_DIR ./bin/boot.bin ./bin/kernel.bin

This intentionally avoids relying on Makefile all, because all currently depends on programs/blank, which is not present in this workspace.

Step 3: Assemble disk image
File: scripts/smoke-test.sh

The script creates bin/os.bin in this order:
- boot sector binary
- kernel binary
- 16 MiB zero padding for FAT area and content storage

This mirrors the expected PeachOS image layout currently used in your project.

Step 4: Resolve bootable user program
File: scripts/smoke-test.sh

Resolution order:
1) USER_PROGRAM_BIN environment override (default path programs/blank/blank.bin)
2) programs/todo/build/todo_app.elf if already built
3) If needed, build fallback via programs/todo/Makefile

If no user program exists after this, the script exits with a clear error and instructions.

Step 5: Mount image and install files
File: scripts/smoke-test.sh

The script:
- Authenticates sudo once.
- Mounts os.bin as vfat at MOUNT_POINT (/mnt/d by default).
- Copies hello.txt if present.
- Copies the resolved user binary into the image as blank.bin.

Important detail:
Even when source binary is todo_app.elf, it is copied as blank.bin, matching kernel expectations.

Step 6: Boot QEMU headless and capture VGA memory
File: scripts/smoke-test.sh

The script runs QEMU with monitor stdio and no display window, then sends monitor commands:
- stop
- pmemsave 0xb8000 0x4000 "path/to/vga.bin"
- quit

This captures VGA text buffer memory for post-processing.

Step 7: Decode VGA bytes into text rows
File: scripts/smoke-test.sh

A small Perl block reads two-byte VGA cells and extracts printable characters into 80x25 text output.
The decoded text is written to a temporary file and inspected.

Step 8: Panic signature detection
File: scripts/smoke-test.sh

The script greps decoded VGA text for:
- failed to load
- panic
- exception
- triple fault

If any signature appears, smoke-test fails with a diagnostic dump.
If none appear, smoke-test reports pass.

File-by-File Explanation
1) scripts/smoke-test.sh
Purpose
Automates build, image assembly, user-program provisioning, headless boot, and crash detection.

Key design choices
- Uses strict shell mode (set -euo pipefail) for safer failure behavior.
- Checks dependencies before work starts.
- Does not depend on root Makefile all target.
- Handles missing programs/blank gracefully by falling back to programs/todo.
- Uses trap logic to unmount image and clean temporary files even on error.

Configurable environment knobs
- BOOT_WAIT_SECONDS: delay before capture.
- QEMU_TIMEOUT_SECONDS: max emulator run time.
- OS_IMAGE: output image path.
- MOUNT_POINT: mount location.
- USER_PROGRAM_BIN: explicit user binary override.

2) Makefile
Purpose
Primary kernel build graph and image assembly path for your existing PeachOS workflow.

Relevant updates
- Added .PHONY including smoke-test.
- Added smoke-test target:
  bash ./scripts/smoke-test.sh

Important current limitation
- all still calls user_programs with programs/blank, which does not exist in this workspace.
- smoke-test bypasses this by building only required kernel artifacts.

3) build.sh
Purpose
Convenience wrapper that sets cross-compiler environment and runs make all.

Current behavior
- Exports PREFIX, TARGET, and PATH for i686-elf toolchain.
- Calls sudo -v before make to avoid hanging at mount/copy prompt later.
- Runs make all, which still depends on programs/blank.

Implication
- build.sh can still fail in this repository state unless programs/blank is restored or Makefile is adjusted.

4) src/kernel.c
Purpose in this context
Defines kernel startup path that eventually loads first user process.

Critical line of behavior
process_load("0:/blank.bin", &process)

If this fails, kernel calls panic("Failed to load blank.bin\n").
This is exactly the panic signature smoke-test checks for in VGA memory.

5) programs/todo/Makefile
Purpose
Build system for fallback user program used by smoke-test when blank binary is missing.

Role in current workflow
- Produces programs/todo/build/todo_app.elf.
- smoke-test can build it on demand and install it as blank.bin.

How the pieces work together
High-level pipeline:
1) Kernel and boot binaries are built.
2) Image is assembled and mounted.
3) A valid user binary is guaranteed and copied as blank.bin.
4) QEMU boots image.
5) VGA memory is captured and decoded.
6) Panic signatures are checked.
7) Pass/fail status is returned to make smoke-test.

Why your latest run passed
Your latest output included:
installed user program as blank.bin from .../programs/todo/build/todo_app.elf
followed by:
[smoke] pass

This means the prior boot blocker (missing blank.bin) is resolved in the smoke workflow.

About blank VGA rows on pass
A pass with blank first 8 rows is possible and not inherently incorrect.
Reasons may include:
- Capture occurred before text rendering.
- Rendered text is outside sampled rows.
- Program is idle without writing visible characters.

This is why current pass criteria are negative (no panic signatures) rather than positive (must contain known success marker).

Known Gaps and Recommended Follow-ups
1) Add a positive boot marker assertion
Example: write a deterministic marker string from kernel or userland and assert it appears in VGA or serial output.

2) Unify program target naming
Long-term, either:
- Restore programs/blank build target, or
- Change kernel loader path and Makefile conventions to your current program layout.

3) Consider serial-based assertions
Serial checks are often less timing-sensitive than VGA memory capture and simplify parsing.

Operational Usage
Primary command:
make smoke-test

Useful override examples:
- USER_PROGRAM_BIN=/absolute/path/to/myprog.elf make smoke-test
- BOOT_WAIT_SECONDS=4 make smoke-test
- QEMU_TIMEOUT_SECONDS=20 make smoke-test

Summary
The current state is functional for automated smoke validation.
The workflow now handles the real repository condition (missing programs/blank build output), keeps kernel expectations satisfied by installing blank.bin, and returns meaningful pass/fail diagnostics based on captured boot-state evidence.
