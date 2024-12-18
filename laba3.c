#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/timer.h>

#define DEVICE_NAME "my_timer"

static int major_num;


struct my_device {
    struct timer_list timer;
    int timer_value;
};
static struct my_device *my_dev;

static void timer_function(struct timer_list *t)
{
    struct my_device *dev = from_timer(dev, t, timer);
    dev->timer_value++;
    printk(KERN_INFO "Timer value: %d\n", dev->timer_value);
    mod_timer(t, jiffies + msecs_to_jiffies(1000));
}

static ssize_t my_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    return sprintf(buf, "%d\n", my_dev->timer_value);
}

static ssize_t my_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    if (!strcmp(buf, "reset\n")) {
        my_dev->timer_value = 0;
        return count;
    } else if (!strcmp(buf, "start\n")) {
        mod_timer(&my_dev->timer, jiffies + msecs_to_jiffies(1000));
        return count;
    } else if (!strcmp(buf, "stop\n")) {
        del_timer(&my_dev->timer);
        return count;
    }
    return -EINVAL;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = my_read,
    .write = my_write,
};

static int __init my_init(void)
{
    major_num = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_num < 0) {
        printk(KERN_ALERT "Registering char device failed\n");
        return major_num;
    }

    printk(KERN_INFO "I was assigned major number %d.  Try cat /dev/%s\n", major_num, DEVICE_NAME);

    my_dev = kzalloc(sizeof(*my_dev), GFP_KERNEL);
    if (!my_dev) {
        unregister_chrdev(major_num, DEVICE_NAME);
        return -ENOMEM;
    }

    timer_setup(&my_dev->timer, timer_function, 0);

    return 0;
}

static void __exit my_exit(void)
{
    unregister_chrdev(major_num, DEVICE_NAME);
    del_timer(&my_dev->timer);
    kfree(my_dev);
    printk(KERN_INFO "Goodbye from %s\n", DEVICE_NAME);
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");
