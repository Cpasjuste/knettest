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


void log_write_len(const char *buffer, SceSize len) {
    SceUID fd = ksceIoOpen(LOG_FILE,
                           SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 6);
    if (fd < 0)
        return;

    ksceIoWrite(fd, buffer, len);
    ksceIoClose(fd);
}

static SceUID kpool_uid = -1;

void *kpool_alloc(int size) {

    void *kpool = NULL;

    /*
    SceKernelAllocMemBlockKernelOpt opt;
    memset(&opt, 0, sizeof(opt));
    opt.size = sizeof(opt);
    opt.attr = 0xA0000000 | 0x400000;
    opt.alignment = 0x40;
    opt.attr |= SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_ALIGNMENT;
    */

    kpool_uid = ksceKernelAllocMemBlock("kpool",
                                        SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW,
                                        ALIGN(size, 0xfff),
                                        0);
    if (kpool_uid >= 0) {
        if (ksceKernelGetMemBlockBase(kpool_uid, &kpool) < 0) {
            LOG("ksceKernelGetMemBlockBase failed\n");
            return NULL;
        }
    } else {
        LOG("ksceKernelAllocMemBlock failed\n");
        return NULL;
    }

    return kpool;
}

void kpool_free() {
    if (kpool_uid >= 0) {
        ksceKernelFreeMemBlock(kpool_uid);
    }
}
