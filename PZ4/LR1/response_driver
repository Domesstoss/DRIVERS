#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/signal.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/ktime.h>
#include <linux/file.h>
#include <linux/path.h>
#include <linux/namei.h>

#define DRIVER_NAME "response_analyzer"
#define IOCTL_PID_SET _IOW('a', 'b', pid_t)
#define IOCTL_RECEIVED_SIGNAL _IO('a', 'd')
#define IOCTL_EXIT _IO('a', 'e')

#define MODELING_TIME_SECONDS 60*5 // 5 минут

static pid_t process_id = -1;
static int device_usage_counter = 0;
static int major_number;
static struct class *device_class;

struct timer_list response_timer;
unsigned long timer_period_ms = 1000;
ktime_t signal_sent_time;

// Имя файла для сохранения данных
static char *output_file_path = "/tmp/response_times.txt";
module_param(output_file_path, charp, 0000);
MODULE_PARM_DESC(output_file_path, "Path to the output file for response times");

int time_measurements_count = 0;

void response_timer_function(struct timer_list *t) {
    if (process_id != -1) {
        struct pid *pid_reference = find_get_pid(process_id);
        if (pid_reference) {
            struct task_struct *task_reference = pid_task(pid_reference, PIDTYPE_PID);
            if (task_reference) {
                send_sig(SIGUSR1, task_reference, 0);
                signal_sent_time = ktime_get_real();
            } else {
                pr_alert("Task not found for PID: %d\n", process_id);
            }
            put_pid(pid_reference);
        } else {
            pr_alert("Invalid PID: %d\n", process_id);
        }
    }
    mod_timer(&response_timer, jiffies + msecs_to_jiffies(timer_period_ms));
}

static void create_and_write_to_file(const char *filename, const char *data){
    struct file *file;
    struct path path;
    loff_t pos = 0;
    int err;
    
    // Получаем путь к файлу по его имени
    err = kern_path(filename, LOOKUP_FOLLOW, &path);
    if (err) {
        pr_err("Failed to get path for %s\n", filename);
        return;
    }

    // Открываем файл для записи (O_CREAT | O_WRONLY | O_APPEND)
    file = file_open_root(path.dentry, path.mnt, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (IS_ERR(file)) {
        pr_err("Failed to open file %s\n", filename);
        path_put(&path);
        return;
    }

    // Записываем данные в файл
    kernel_write(file, data, strlen(data), &pos);

    // Закрываем файл
    fput(file);

    // Освобождаем ресурсы
    path_put(&path);
}

static int open_device(struct inode *inode_ptr, struct file *file_ptr) {
    if (device_usage_counter)
        return -EBUSY;
    device_usage_counter++;
    pr_alert("Device opened\n");
    return 0;
}

static int close_device(struct inode *inode_ptr, struct file *file_ptr) {
    if (device_usage_counter <= 0)
        return -EBUSY;
    device_usage_counter--;
    process_id = -1;
    pr_alert("Device closed\n");
    return 0;
}

static long ioctl_device(struct file *file_ptr, unsigned int ioctl_command, unsigned long ioctl_param) {
    switch (ioctl_command) {
        case IOCTL_PID_SET:
            copy_from_user(&process_id, (pid_t *)ioctl_param, sizeof(pid_t));
            break;
        case IOCTL_RECEIVED_SIGNAL:
            {
                ktime_t signal_received_time = ktime_get_real();
                s64 time_difference = ktime_to_ns(ktime_sub(signal_received_time, signal_sent_time));

                // Запись времени отклика в файл
                char data_buffer[128];
                int len = snprintf(data_buffer, sizeof(data_buffer), "%lld\n", time_difference);
                create_and_write_to_file(output_file_path, data_buffer);

                time_measurements_count++;

                if (time_measurements_count == MODELING_TIME_SECONDS) {
                    if (process_id != -1) {
                        struct pid *pid_reference = find_get_pid(process_id);
                        if (pid_reference) {
                            struct task_struct *task_reference = pid_task(pid_reference, PIDTYPE_PID);
                            if (task_reference) {
                                send_sig(SIGINT, task_reference, 0);
                            } else {
                                pr_alert("Task not found for PID: %d\n", process_id);
                            }
                            put_pid(pid_reference);
                        } else {
                            pr_alert("Invalid PID: %d\n", process_id);
                        }
                    }
                }
            }
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

static struct file_operations file_ops = {
    .open = open_device,
    .release = close_device,
    .unlocked_ioctl = ioctl_device,
};

static int __init response_analyzer_init(void) {
    // Регистрация старшего номера устройства
    major_number = register_chrdev(0, DRIVER_NAME, &file_ops);
    if (major_number < 0) {
        pr_alert("register_chrdev() failed: %d\n", major_number);
        return -EINVAL;
    }
    pr_info("major = %d\n", major_number);

    // Создание класса устройств
    device_class = class_create(DRIVER_NAME);
    if (IS_ERR(device_class)) {
        pr_alert("class_create() failed: %ld\n", PTR_ERR(device_class));
        return -EINVAL;
    }

    // Создание устройства
    struct device* dev = device_create(device_class, NULL, MKDEV(major_number, 0), NULL, DRIVER_NAME);
    if (IS_ERR(dev)) {
        pr_alert("device_create() failed: %ld\n", PTR_ERR(dev));
        return -EINVAL;
    }

    pr_info("/dev/%s created\n", DRIVER_NAME);

    // Инициализация и запуск таймера
    timer_setup(&response_timer, response_timer_function, 0);
    mod_timer(&response_timer, jiffies + msecs_to_jiffies(timer_period_ms));

    return 0;
}

static void __exit response_analyzer_exit(void) {
    // Удаление таймера
    del_timer(&response_timer);

    // Удаление устройства
    device_destroy(device_class, MKDEV(major_number, 0));

    // Удаление класса устройств
    class_destroy(device_class);

    // Отмена регистрации старшего номера устройства
    unregister_chrdev(major_number, DRIVER_NAME);
}

module_init(response_analyzer_init);
module_exit(response_analyzer_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Response time measurement driver");
MODULE_AUTHOR("Dima");
