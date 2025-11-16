/*
 * Memory Character Device Driver
 * 
 * This driver creates a character device /dev/mymem that:
 * - Reading from it returns system memory usage from /proc/meminfo
 * - Writing to it is ignored
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/seq_file.h>

#define DEVICE_NAME "mymem"
#define CLASS_NAME "mymem"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple memory character device driver");
MODULE_VERSION("1.0");

static int major_number;
static struct class *mymem_class = NULL;
static struct device *mymem_device = NULL;

// Buffer to store memory info
#define MEMINFO_BUFFER_SIZE 4096
static char *meminfo_buffer = NULL;

/*
 * Read memory info from /proc/meminfo
 * Returns the number of bytes read, or negative on error
 */
static ssize_t read_meminfo(char *buffer, size_t count)
{
    struct file *meminfo_file;
    mm_segment_t old_fs;
    loff_t pos = 0;
    ssize_t bytes_read = 0;

    // Open /proc/meminfo
    meminfo_file = filp_open("/proc/meminfo", O_RDONLY, 0);
    if (IS_ERR(meminfo_file)) {
        printk(KERN_ERR "mymem: Failed to open /proc/meminfo\n");
        return PTR_ERR(meminfo_file);
    }

    // Switch to kernel space for file operations
    old_fs = get_fs();
    set_fs(KERNEL_DS);

    // Read from the file
    if (meminfo_buffer) {
        kfree(meminfo_buffer);
    }
    meminfo_buffer = kmalloc(MEMINFO_BUFFER_SIZE, GFP_KERNEL);
    if (!meminfo_buffer) {
        filp_close(meminfo_file, NULL);
        set_fs(old_fs);
        return -ENOMEM;
    }

    bytes_read = kernel_read(meminfo_file, meminfo_buffer, MEMINFO_BUFFER_SIZE - 1, &pos);
    set_fs(old_fs);

    filp_close(meminfo_file, NULL);

    if (bytes_read < 0) {
        kfree(meminfo_buffer);
        meminfo_buffer = NULL;
        return bytes_read;
    }

    // Null-terminate the buffer
    meminfo_buffer[bytes_read] = '\0';

    // Copy to user space (limited by count)
    if (bytes_read > count) {
        bytes_read = count;
    }

    if (copy_to_user(buffer, meminfo_buffer, bytes_read)) {
        return -EFAULT;
    }

    return bytes_read;
}

/*
 * File operations: open
 */
static int mymem_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "mymem: Device opened\n");
    return 0;
}

/*
 * File operations: release
 */
static int mymem_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "mymem: Device closed\n");
    return 0;
}

/*
 * File operations: read
 * Returns memory info from /proc/meminfo
 */
static ssize_t mymem_read(struct file *file, char __user *buffer, size_t count, loff_t *offset)
{
    ssize_t bytes_read;

    if (*offset > 0) {
        // If we've already read everything, return 0
        return 0;
    }

    bytes_read = read_meminfo(buffer, count);
    
    if (bytes_read > 0) {
        *offset += bytes_read;
    }

    return bytes_read;
}

/*
 * File operations: write
 * Writing is ignored
 */
static ssize_t mymem_write(struct file *file, const char __user *buffer, size_t count, loff_t *offset)
{
    printk(KERN_INFO "mymem: Write operation ignored (count=%zu)\n", count);
    // Writing is ignored, return the count as if we wrote it
    return count;
}

/*
 * File operations structure
 * This is the core structure that defines all operations on the device
 */
static const struct file_operations mymem_fops = {
    .owner = THIS_MODULE,
    .open = mymem_open,
    .release = mymem_release,
    .read = mymem_read,
    .write = mymem_write,
};

/*
 * Module initialization
 */
static int __init mymem_init(void)
{
    printk(KERN_INFO "mymem: Initializing memory character device driver\n");

    // Allocate major number
    major_number = register_chrdev(0, DEVICE_NAME, &mymem_fops);
    if (major_number < 0) {
        printk(KERN_ALERT "mymem: Failed to register a major number\n");
        return major_number;
    }
    printk(KERN_INFO "mymem: Registered correctly with major number %d\n", major_number);

    // Create device class
    mymem_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(mymem_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "mymem: Failed to register device class\n");
        return PTR_ERR(mymem_class);
    }
    printk(KERN_INFO "mymem: Device class registered correctly\n");

    // Create the device
    mymem_device = device_create(mymem_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(mymem_device)) {
        class_destroy(mymem_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "mymem: Failed to create the device\n");
        return PTR_ERR(mymem_device);
    }
    printk(KERN_INFO "mymem: Device created successfully\n");
    printk(KERN_INFO "mymem: Use 'cat /dev/mymem' to read memory info\n");

    return 0;
}

/*
 * Module cleanup
 */
static void __exit mymem_exit(void)
{
    // Clean up buffer
    if (meminfo_buffer) {
        kfree(meminfo_buffer);
        meminfo_buffer = NULL;
    }

    // Remove device
    device_destroy(mymem_class, MKDEV(major_number, 0));
    printk(KERN_INFO "mymem: Device removed\n");

    // Unregister device class
    class_unregister(mymem_class);
    class_destroy(mymem_class);
    printk(KERN_INFO "mymem: Device class unregistered\n");

    // Unregister major number
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "mymem: Unregistered major number %d\n", major_number);

    printk(KERN_INFO "mymem: Memory character device driver removed\n");
}

module_init(mymem_init);
module_exit(mymem_exit);

