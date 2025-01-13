#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#define DEVICE_PATH "/dev/response_analyzer"
#define IOCTL_PID_SET _IOW('a', 'b', pid_t)
#define IOCTL_RECEIVED_SIGNAL _IO('a', 'd')
#define IOCTL_EXIT _IO('a', 'e')

int device_file_descriptor;

void handle_driver_signal(int signal_number) {
    printf("Received signal from driver!\n");
    // Сигнализируем драйверу о приеме
    if (device_file_descriptor < 0) {
        perror("Device failed to open");
        return;
    }
    ioctl(device_file_descriptor, IOCTL_RECEIVED_SIGNAL);
}

void handle_program_exit(int signal_number) {
    printf("Exit!\n");
    close(device_file_descriptor);
    exit(0);
}

int main() {
    // Регистрация обработчика SIGINT
    signal(SIGINT, handle_program_exit);

    // Открытие файла устройства
    device_file_descriptor = open(DEVICE_PATH, O_RDWR);
    if (device_file_descriptor < 0) {
        perror("Failed to open device");
        return -1;
    }

    // Получение PID текущего процесса
    pid_t process_id = getpid();

    // Регистрация обработчика сигнала от драйвера
    signal(SIGUSR1, handle_driver_signal);

    // Установка PID процесса в драйвере
    ioctl(device_file_descriptor, IOCTL_PID_SET, &process_id);

    // Бесконечный цикл ожидания сигнала
    while (1) {
        pause(); // Приостановка выполнения до получения сигнала
    }

    // Закрытие файла устройства (этот код никогда не выполнится в данном случае)
    close(device_file_descriptor);
    return 0;
}
