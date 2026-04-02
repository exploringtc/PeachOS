#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

BOOT_WAIT_SECONDS="${BOOT_WAIT_SECONDS:-2}"
QEMU_TIMEOUT_SECONDS="${QEMU_TIMEOUT_SECONDS:-12}"
OS_IMAGE="${OS_IMAGE:-${ROOT_DIR}/bin/os.bin}"
MOUNT_POINT="${MOUNT_POINT:-/mnt/d}"
USER_PROGRAM_BIN="${USER_PROGRAM_BIN:-${ROOT_DIR}/programs/blank/blank.bin}"
TODO_PROGRAM_BIN="${ROOT_DIR}/programs/todo/build/todo_app.elf"

require_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "missing required command: $1" >&2
        exit 1
    fi
}

require_cmd timeout
require_cmd qemu-system-i386
require_cmd perl

export PREFIX="${PREFIX:-$HOME/opt/cross}"
export TARGET="${TARGET:-i686-elf}"
export PATH="$PREFIX/bin:$PATH"

require_cmd i686-elf-gcc
require_cmd i686-elf-ld
require_cmd nasm
require_cmd dd
require_cmd sudo

echo "[smoke] authenticating sudo for image mount"
sudo -v

echo "[smoke] building bootloader and kernel"
make -C "$ROOT_DIR" ./bin/boot.bin ./bin/kernel.bin

echo "[smoke] assembling OS image"
rm -f "$OS_IMAGE"
dd if="${ROOT_DIR}/bin/boot.bin" of="$OS_IMAGE" status=none
dd if="${ROOT_DIR}/bin/kernel.bin" of="$OS_IMAGE" oflag=append conv=notrunc status=none
dd if=/dev/zero of="$OS_IMAGE" bs=1048576 count=16 oflag=append conv=notrunc status=none

resolved_user_program=""
if [ -f "$USER_PROGRAM_BIN" ]; then
    resolved_user_program="$USER_PROGRAM_BIN"
elif [ -f "$TODO_PROGRAM_BIN" ]; then
    resolved_user_program="$TODO_PROGRAM_BIN"
elif [ -f "${ROOT_DIR}/programs/todo/Makefile" ]; then
    echo "[smoke] building fallback user program from programs/todo"
    make -C "${ROOT_DIR}/programs/todo" all
    if [ -f "$TODO_PROGRAM_BIN" ]; then
        resolved_user_program="$TODO_PROGRAM_BIN"
    fi
fi

if [ -z "$resolved_user_program" ]; then
    echo "[smoke] no user program available for kernel loader (expected blank.bin)" >&2
    echo "[smoke] build or provide one using USER_PROGRAM_BIN=/path/to/program make smoke-test" >&2
    exit 1
fi

mounted=0
cleanup_mount() {
    if [ "$mounted" -eq 1 ]; then
        sudo umount "$MOUNT_POINT" >/dev/null 2>&1 || true
    fi
}
trap cleanup_mount EXIT

sudo mkdir -p "$MOUNT_POINT"
sudo mount -t vfat "$OS_IMAGE" "$MOUNT_POINT"
mounted=1

if [ -f "${ROOT_DIR}/hello.txt" ]; then
    sudo cp "${ROOT_DIR}/hello.txt" "$MOUNT_POINT"
fi

sudo cp "$resolved_user_program" "$MOUNT_POINT/blank.bin"
echo "[smoke] installed user program as blank.bin from: $resolved_user_program"

sudo umount "$MOUNT_POINT"
mounted=0

if [ ! -f "$OS_IMAGE" ]; then
    echo "[smoke] expected image not found: $OS_IMAGE" >&2
    exit 1
fi

tmp_dir="$(mktemp -d)"
trap 'cleanup_mount; rm -rf "$tmp_dir"' EXIT

qemu_log="$tmp_dir/qemu.log"
vga_dump="$tmp_dir/vga.bin"
vga_text="$tmp_dir/vga.txt"

echo "[smoke] booting QEMU headless and capturing VGA memory"
set +e
{
    sleep "$BOOT_WAIT_SECONDS"
    echo "stop"
    echo "pmemsave 0xb8000 0x4000 \"$vga_dump\""
    echo "quit"
} | timeout "${QEMU_TIMEOUT_SECONDS}s" qemu-system-i386 \
    -drive file="$OS_IMAGE",format=raw,if=ide \
    -display none \
    -serial none \
    -monitor stdio \
    -no-reboot \
    -no-shutdown >"$qemu_log" 2>&1
qemu_exit=$?
set -e

if [ "$qemu_exit" -ne 0 ]; then
    echo "[smoke] QEMU failed with exit code $qemu_exit" >&2
    tail -n 80 "$qemu_log" >&2 || true
    exit 1
fi

if [ ! -s "$vga_dump" ]; then
    echo "[smoke] failed to capture VGA memory" >&2
    tail -n 80 "$qemu_log" >&2 || true
    exit 1
fi

perl -e '
    use strict;
    use warnings;

    my ($in, $out) = @ARGV;
    open my $fh, "<:raw", $in or die "open input: $!";
    local $/;
    my $data = <$fh>;
    close $fh;

    open my $ofh, ">", $out or die "open output: $!";
    my $width = 80;
    my $height = 25;
    for my $row (0 .. $height - 1) {
        my $line = "";
        for my $col (0 .. $width - 1) {
            my $idx = ($row * $width + $col) * 2;
            my $ch = ord(substr($data, $idx, 1) // "\0");
            if ($ch >= 32 && $ch <= 126) {
                $line .= chr($ch);
            } else {
                $line .= " ";
            }
        }
        $line =~ s/\s+$//;
        print {$ofh} "$line\n";
    }
    close $ofh;
' "$vga_dump" "$vga_text"

if grep -Eqi 'failed to load|panic|exception|triple fault' "$vga_text"; then
    echo "[smoke] detected panic-like text on VGA" >&2
    sed -n '1,25p' "$vga_text" >&2
    exit 1
fi

echo "[smoke] pass"
echo "[smoke] VGA snapshot (first 8 rows):"
sed -n '1,8p' "$vga_text"