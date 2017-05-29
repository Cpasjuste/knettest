//
// Created by cpasjuste on 29/05/17.
//

#ifndef NETTEST_UTILS_H
#define NETTEST_UTILS_H

#define LOG_FILE "ux0:tai/nettest.log"
#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

void log_write(const char *buffer);
void *kpool_alloc(int size);
void kpool_free();

#define LOG(...) \
    do { \
        char buffer[256]; \
        snprintf(buffer, sizeof(buffer), ##__VA_ARGS__); \
        log_write(buffer); \
    } while (0)

#endif //NETTEST_UTILS_H
