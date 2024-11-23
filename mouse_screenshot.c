#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/fs.h>
#include <linux/signal.h>
#include <linux/uaccess.h>
#include <linux/fcntl.h>
#include <linux/poll.h>
#include <linux/types.h>

#define DEVICE_NAME "mouse_screenshot"
static int click_count = 0; // To count right-clicks
static struct fasync_struct *async_queue; // For user-space notifications

// Event filter to detect mouse events
static bool mouse_event(struct input_handle *handle, unsigned int type, unsigned int code, int value) {
    if (type == EV_KEY && code == BTN_RIGHT && value == 1) { // Right button pressed
        click_count++;
        if (click_count == 3) {
            printk(KERN_INFO "Right mouse button clicked three times!\n");
            if (async_queue)
                kill_fasync(&async_queue, SIGIO, POLL_IN); // Notify user-space
            click_count = 0; // Reset the click count
        }
    }
    return false; // Pass the event to other handlers
}

// Input device connect function
static int mouse_connect(struct input_handler *handler, struct input_dev *dev, const struct input_device_id *id) {
    struct input_handle *handle;

    printk(KERN_INFO "Mouse device connected: %s\n", dev->name);

    handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
    if (!handle)
        return -ENOMEM;

    handle->dev = dev;
    handle->handler = handler;
    handle->name = DEVICE_NAME;

    return input_register_handle(handle);
}

// Input device disconnect function
static void mouse_disconnect(struct input_handle *handle) {
    printk(KERN_INFO "Mouse device disconnected: %s\n", handle->dev->name);
    input_unregister_handle(handle);
    kfree(handle);
}

// File operations for the device
static int mouse_fasync(int fd, struct file *file, int mode) {
    return fasync_helper(fd, file, mode, &async_queue);
}

static int mouse_open(struct inode *inode, struct file *file) {
    return 0;
}

static int mouse_release(struct inode *inode, struct file *file) {
    mouse_fasync(-1, file, 0);
    return 0;
}

static struct file_operations mouse_fops = {
    .owner = THIS_MODULE,
    .fasync = mouse_fasync,
    .open = mouse_open,
    .release = mouse_release,
};

// Input handler definition
static const struct input_device_id mouse_ids[] = {
    { .driver_info = 1 }, // Match all input devices
    { },                  // Terminating entry
};

MODULE_DEVICE_TABLE(input, mouse_ids);

static struct input_handler mouse_handler = {
    .filter = mouse_event,
    .connect = mouse_connect,
    .disconnect = mouse_disconnect,
    .name = DEVICE_NAME,
    .id_table = mouse_ids,
};

// Module initialization
static int __init mouse_init(void) {
    int ret;

    ret = input_register_handler(&mouse_handler);
    if (ret) {
        printk(KERN_ERR "Failed to register input handler\n");
        return ret;
    }

    printk(KERN_INFO "Mouse screenshot module loaded\n");
    return 0;
}

// Module exit
static void __exit mouse_exit(void) {
    input_unregister_handler(&mouse_handler);
    printk(KERN_INFO "Mouse screenshot module unloaded\n");
}

module_init(mouse_init);
module_exit(mouse_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Haseeb");
MODULE_DESCRIPTION("Kernel module to detect triple right-click and notify user-space.");

