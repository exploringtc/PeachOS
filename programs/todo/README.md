# CYSE 570 Final Project — To-Do List OS Application

**Course:** CYSE 570 - Operating Systems Fundamentals  
**Institution:** George Mason University  
**Semester:** Spring 2026  
**Team:** Mustafa Badaoui & Thomas Cho  
**GitHub Repository:** https://github.com/exploringtc/cyse570finalproject  
**Base OS:** [PeachOS](https://github.com/nibblebits/PeachOS)

---

## Overview

This project extends the **PeachOS** custom kernel by implementing a terminal-based task management application that runs entirely within the OS's program space. Users can create, view, delete, and persist task lists — all secured by a lightweight, reversible XOR-based encryption scheme.

The application interfaces directly with the kernel through system calls, interrupts, and process management to access system resources. The centerpiece feature is **secure persistence**: tasks are saved to disk in encrypted form using XOR encoding with a user-provided key. When the system boots and a saved file is loaded, the task list is decrypted and reconstructed in memory.

---

## Features

- Add, list, and remove tasks via a command-line interface
- In-memory task management using a dynamic data structure (array or linked list)
- Each task tracks: **ID**, **Description**, and **Status** (complete / incomplete)
- Save tasks to disk with **XOR encryption** using a user-provided key
- Load and decrypt tasks from disk back into memory
- Runs entirely in user-space on the PeachOS kernel — no standard library required
- Process isolation via `fork()` — application runs as a child process of the shell

---

## Command Interface

| Command | Description |
|---|---|
| `add <task>` | Adds a new task to the in-memory list |
| `list` | Displays all tasks with their ID and status |
| `remove <id>` | Deletes a task by its unique ID |
| `complete <id>` | Marks a task as completed |
| `uncomplete <id>` | Marks a task as incomplete |
| `save <filename> <key>` | Encrypts and writes the task list to disk |
| `load <filename> <key>` | Reads and decrypts a task list from disk |

---

## Application Operation — Step by Step

### 1. System Startup
- The kernel boots and initializes memory, paging, and process structures.
- The to-do application is loaded from the `/programs` directory.

### 2. User Interface
- The user interacts through a single terminal command-line interface.
- Keyboard input is processed via the keyboard interrupt handler (see [Interrupt Operation](#interrupt-operation)).

### 3. Task Management (In-Memory)
- Tasks are stored in a dynamic in-memory structure.
- Each task contains: a unique **ID**, a **description** string, and a **status** flag.

### 4. Saving Tasks (Encryption + Disk Write)
When `save <filename>` is called:
1. Tasks are serialized into a byte buffer.
2. XOR encryption is applied using the user-provided key:
   ```
   encrypted_data[i] = data[i] ^ key[i % key_length]
   ```
3. The encrypted buffer is written to disk through the kernel disk driver.

### 5. Loading Tasks (Disk Read + Decryption)
When `load <filename>` is called:
1. The encrypted file is read from disk into a buffer.
2. XOR decryption is applied using the same key (XOR is its own inverse):
   ```
   original_data[i] = encrypted_data[i] ^ key[i % key_length]
   ```
3. The decrypted data is reconstructed into task structures in memory.

---

## System Call Operations

### `sys_add_task(char *task)`
Adds a new task to the in-memory task list.
- Validates the user-space pointer `char *task`
- Allocates kernel memory for the new task
- Assigns a unique ID and inserts it into the task list
- Demonstrates safe memory manipulation and user→kernel pointer validation

### `sys_list_tasks()`
Displays all tasks currently stored in memory.
- Iterates through the task list
- Prints each task's ID, description, and status to the terminal via the kernel print function
- Demonstrates read-only access to kernel-managed data

### `sys_remove_task(int id)`
Deletes a task by its unique ID.
- Searches for a task matching the given ID
- If found: removes it from the data structure and frees associated memory
- If not found: returns an error message
- Demonstrates memory deallocation and safe modification of kernel data structures

### `sys_save_tasks(char *filename, char *key)`
Saves the current task list to disk using XOR encryption.
- Serializes tasks into a byte buffer
- Applies XOR encryption with the provided key
- Calls the kernel disk driver to create/open the file and write the encrypted data
- Demonstrates file I/O through the kernel, basic encryption, and data persistence

### `sys_load_tasks(char *filename, char *key)`
Loads and restores tasks from an encrypted file on disk.
- Reads the file from disk into a buffer
- Applies XOR decryption using the same key
- Reconstructs task structures in memory from the decrypted data
- Demonstrates file reading, data reconstruction, and reversible encryption

---

## Interrupt Operation

**Keyboard Interrupt (IRQ1):**
1. User presses a key → hardware interrupt is generated
2. The CPU pauses the current execution flow
3. The keyboard interrupt handler is invoked
4. Keystrokes are registered and processed
5. Characters are forwarded to the terminal input buffer
6. Control returns to the running application

---

## Process Operation — `fork()`

1. The terminal shell acts as the **parent process**
2. When the to-do application is launched, the shell calls `fork()`
3. A **child process** is created as a copy of the parent
4. The child process executes the to-do application
5. The parent process either waits for the child to finish or continues handling future shell input

---

## Page Table / Memory Management

| Event | Memory Behavior |
|---|---|
| Application startup | OS maps memory pages for the new process |
| `add <task>` | Additional memory allocated for task strings and input buffers |
| `save` / `load` | Temporary I/O buffers placed in memory |
| `fork()` | Child process receives its own address space (copied or copy-on-write) |

Process isolation via the page table ensures that one process cannot interfere with another's memory, supporting safer multitasking on the kernel.

---

## Project Structure

```
cyse570finalproject/
├── src/
│   ├── main.c          # Application entry point and command loop
│   ├── todo.c          # Task management logic (add, list, remove)
│   ├── todo.h          # Task data structures and function declarations
│   ├── syscall.c       # System call wrappers for PeachOS
│   ├── encrypt.c       # XOR encryption / decryption logic
│   └── disk.c          # Disk I/O helpers (save/load via kernel driver)
├── Makefile            # Build system
└── README.md
```

---

## Requirements

- **PeachOS** built and running (see the [PeachOS repository](https://github.com/nibblebits/PeachOS))
- `nasm` — assembler for x86 code
- `i686-elf-gcc` — cross-compiler targeting x86 bare-metal
- `qemu-system-i386` — emulator for testing
- GNU `make`

---

## Building & Running

1. Clone this repository into the PeachOS user programs directory:
   ```bash
   git clone https://github.com/exploringtc/cyse570finalproject
   cd cyse570finalproject
   ```

2. Build the application:
   ```bash
   make
   ```

3. Rebuild PeachOS to bundle the application into the disk image:
   ```bash
   make -C /path/to/PeachOS
   ```

4. Boot PeachOS in QEMU:
   ```bash
   qemu-system-i386 -hda /path/to/PeachOS/boot.img
   ```

5. Launch the to-do app from the PeachOS shell:
   ```
   > todo
   ```

---

## OS Concepts Demonstrated

| Concept | Implementation |
|---|---|
| System Calls | Five custom syscalls for task and file operations |
| Memory Management | Dynamic allocation/deallocation in kernel space |
| Encryption | XOR-based reversible encryption for data persistence |
| Interrupts | Keyboard IRQ handler for terminal input |
| Process Model | `fork()` — shell spawns child process for the application |
| Page Tables | Per-process address space; isolation between parent and child |
| File I/O | Kernel disk driver used for encrypted save/load |
| VGA Output | Terminal rendering via PeachOS text-mode display |

---

## References

- [PeachOS GitHub Repository](https://github.com/nibblebits/PeachOS)
- CYSE 570 Course Materials — George Mason University
- [OSDev Wiki](https://wiki.osdev.org/) — Reference for x86 OS development
- Intel 80386 Programmer's Reference Manual