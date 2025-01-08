#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/moduleparam.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dima");
MODULE_DESCRIPTION("Simple kernel module with thread and parameters");

static char *log_message = "Hello, kernel!";
module_param(log_message, charp, 0000);
MODULE_PARM_DESC(log_message, "Message to print in the log");

static unsigned int delay_seconds = 1;
module_param(delay_seconds, uint, 0000);
MODULE_PARM_DESC(delay_seconds, "Delay in seconds between log messages");


static struct task_struct *hello_thread;

static int hello_thread_function(void *data) {
    while (!kthread_should_stop()) {
        printk(KERN_INFO "%s\n", log_message);
        msleep(delay_seconds * 1000);
    }
    return 0;
}


static int __init hello_init(void) {
    printk(KERN_INFO "Hello module loaded\n");

    hello_thread = kthread_run(hello_thread_function, NULL, "hello_thread");
    if (IS_ERR(hello_thread)) {
        printk(KERN_ERR "Failed to create kernel thread\n");
        return PTR_ERR(hello_thread);
    }

    return 0;
}


static void __exit hello_exit(void) {
    if (hello_thread) {
        kthread_stop(hello_thread);
    }
    printk(KERN_INFO "Hello module unloaded\n");
}



module_init(hello_init);
module_exit(hello_exit);
