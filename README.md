# Memory Character Device Driver - Linux Project

A simple Linux kernel module that creates a character device (`/dev/mymem`) for reading system memory information.

## Features

- **Character Device**: Creates `/dev/mymem` device file
- **Read Operation**: Returns system memory usage from `/proc/meminfo`
- **Write Operation**: Writing to the device is ignored (no-op)
- **File Operations Structure**: Demonstrates proper use of `file_operations` structure

## Building the Module

1. Make sure you have kernel headers installed:
   ```bash
   sudo apt-get install linux-headers-$(uname -r)
   ```

2. Build the module:
   ```bash
   make
   ```

## Loading and Using

1. **Load the module**:
   ```bash
   sudo insmod mymem.ko
   ```
   Or use the Makefile shortcut:
   ```bash
   make load
   ```

2. **Read memory information**:
   ```bash
   cat /dev/mymem
   ```

3. **Test write (will be ignored)**:
   ```bash
   echo "test" > /dev/mymem
   cat /dev/mymem  # Still shows memory info
   ```

4. **Unload the module**:
   ```bash
   sudo rmmod mymem
   ```
   Or use the Makefile shortcut:
   ```bash
   make unload
   ```

## Viewing Module Information

Check if the module is loaded:
```bash
lsmod | grep mymem
```

View kernel messages:
```bash
dmesg | tail -20
```

## Code Structure

The driver focuses on the `file_operations` structure which defines:
- `open`: Called when device is opened
- `release`: Called when device is closed
- `read`: Returns memory info from `/proc/meminfo`
- `write`: Ignores write operations (returns count as if written)

## Requirements

- Linux kernel with module support
- Kernel development headers
- Root/sudo access for module loading/unloading

## Notes

- The device is created automatically when the module is loaded
- The device is removed automatically when the module is unloaded
- Reading always returns current memory info from `/proc/meminfo`
- Writing has no effect but doesn't return an error