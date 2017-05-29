//
// Created by cpasjuste on 29/05/17.
//

#include <vitasdkkern.h>
#include <libk/string.h>
#include <libk/stdio.h>
#include "utils.h"

void log_write(const char *buffer) {
    SceUID fd = ksceIoOpen(LOG_FILE,
                           SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 6);
    if (fd < 0)
        return;

    ksceIoWrite(fd, buffer, strlen(buffer));
    ksceIoClose(fd);
}

static SceUID kpool_uid = -1;

void *kpool_alloc(int size) {

    void *kpool = NULL;

    kpool_uid = ksceKernelAllocMemBlock("kpool",
                                        SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW,
                                        ALIGN(size, 0xfff),
                                        0);
    if (kpool_uid >= 0) {
        if (ksceKernelGetMemBlockBase(kpool_uid, &kpool) < 0) {
            LOG("ksceKernelGetMemBlockBase failed\n");
        }
    } else {
        LOG("ksceKernelAllocMemBlock failed\n");
    }

    return kpool;
}

void kpool_free() {
    if (kpool_uid >= 0) {
        ksceKernelFreeMemBlock(kpool_uid);
    }
}
