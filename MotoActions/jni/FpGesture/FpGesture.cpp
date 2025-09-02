#include <android/log.h>
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "FpGesture.h"

#define LOG_TAG "FpGesture"
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#define INPUT_DIR "/dev/input/"
#define DOUBLE_TAP_TIMEOUT 200 // ms

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define LONG(x) ((x)/BITS_PER_LONG)
#define test_bit(bit, array) ((array[LONG(bit)] >> OFF(bit)) & 1)

static bool sIsEnabled = false;
static pthread_t sWorker;
static pthread_mutex_t sLock = PTHREAD_MUTEX_INITIALIZER;
static int sFd = -1;
static long sLastPressTime = 0;

static long getTimeInMillis() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

static char *find_fp_device() {
    char *device_path = NULL;
    DIR *dir = opendir(INPUT_DIR);
    if (dir == NULL) {
        ALOGE("Failed to open input device directory");
        return NULL;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "event", 5) == 0) {
            char path[256];
            snprintf(path, sizeof(path), "%s%s", INPUT_DIR, entry->d_name);
            int fd = open(path, O_RDONLY);
            if (fd >= 0) {
                unsigned long ev_bits[NBITS(EV_MAX)];
                if (ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) >= 0) {
                    if (test_bit(EV_KEY, ev_bits)) {
                        unsigned long key_bits[NBITS(KEY_MAX)];
                        if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) >= 0) {
                            if (test_bit(KEY_AUDIO_DESC, key_bits)) {
                                device_path = strdup(path);
                                close(fd);
                                break;
                            }
                        }
                    }
                }
                close(fd);
            }
        }
    }

    closedir(dir);
    return device_path;
}

static void *worker_thread(void *arg) {
    struct input_event event;

    while (sIsEnabled && sFd >= 0) {
        if (read(sFd, &event, sizeof(event)) == sizeof(event)) {
            if (event.type == EV_KEY && event.code == KEY_AUDIO_DESC && event.value == 1) {
                long pressTime = getTimeInMillis();
                if (pressTime - sLastPressTime < DOUBLE_TAP_TIMEOUT) {
                    on_fp_gesture();
                    sLastPressTime = 0;
                } else {
                    sLastPressTime = pressTime;
                }
            }
        }
    }

    return NULL;
}

void fp_gesture_enable(bool enable) {
    pthread_mutex_lock(&sLock);
    if (enable) {
        if (sIsEnabled) {
            pthread_mutex_unlock(&sLock);
            return;
        }
        char *device_path = find_fp_device();
        if (device_path == NULL) {
            ALOGE("Failed to find fingerprint device");
            pthread_mutex_unlock(&sLock);
            return;
        }
        sFd = open(device_path, O_RDONLY);
        free(device_path);
        if (sFd < 0) {
            ALOGE("Failed to open device");
            pthread_mutex_unlock(&sLock);
            return;
        }
        sIsEnabled = true;
        pthread_create(&sWorker, NULL, worker_thread, NULL);
    } else {
        if (!sIsEnabled) {
            pthread_mutex_unlock(&sLock);
            return;
        }
        sIsEnabled = false;
        if (sFd >= 0) {
            close(sFd);
            sFd = -1;
        }
        pthread_join(sWorker, NULL);
    }
    pthread_mutex_unlock(&sLock);
}