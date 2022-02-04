
#include <libk/string.h>
#include <psp2kern/net/net.h>
#include <psp2common/types.h>
#include <psp2kern/kernel/debug.h>
#include <psp2kern/kernel/threadmgr/thread.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>

#define printf ksceDebugPrintf

#define SERVER_PORT 4444
#define BUFFER_SIZE 16

SceNetSockaddrIn client;
unsigned int clientLen;
int server_sock = -1, client_sock = -1;
SceUID cmd_thid = -1;

int get_sock(int sock) {
    clientLen = sizeof(client);
    return ksceNetAccept(sock, (SceNetSockaddr *) &client, &clientLen);
}

int bind_port(int port) {
    SceNetSockaddrIn serverAddress;
    serverAddress.sin_family = SCE_NET_AF_INET;
    serverAddress.sin_addr.s_addr = SCE_NET_INADDR_ANY;
    serverAddress.sin_port = ksceNetHtons((unsigned short) port);

    // create server socket
    int sock = ksceNetSocket("knet",
                             SCE_NET_AF_INET,
                             SCE_NET_SOCK_STREAM, 0);

    // bind
    if (ksceNetBind(sock, (SceNetSockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
        printf("sceNetBind failed\n");
        return -1;
    }

    // listen
    if (ksceNetListen(sock, 64) < 0) {
        printf("sceNetListen failed\n");
        return -1;
    }

    return sock;
}

void cleanup() {
    printf("cleanup: closing connections\n");
    if (client_sock >= 0) {
        ksceNetSocketClose(client_sock);
    }
    if (server_sock >= 0) {
        ksceNetSocketClose(server_sock);
    }
}

void *kmalloc(size_t size) {
    void *p = NULL;
    SceUID uid = ksceKernelAllocMemBlock(
            "k", SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, (size + 0xFFF) & (~0xFFF), 0);
    if (uid >= 0) {
        ksceKernelGetMemBlockBase(uid, &p);
    }

    return p;
}

void kfree(void *p) {
    SceUID uid = ksceKernelFindMemBlockByAddr(p, 1);
    if (uid >= 0) {
        ksceKernelFreeMemBlock(uid);
    }
}

int cmd_thread(SceSize args, void *argp) {

    server_sock = bind_port(SERVER_PORT);
    if (server_sock <= 0) {
        printf("bind_port failed: %i\n", server_sock);
        cleanup();
        ksceKernelExitDeleteThread(0);
        return -1;
    }

    printf("waiting for clients\n");
    client_sock = get_sock(server_sock);
    if (client_sock <= 0) {
        printf("get_sock failed: %i\n", client_sock);
        cleanup();
        ksceKernelExitDeleteThread(0);
        return -1;
    }

    printf("ksceNetRecvfrom\n");

    char *msg = kmalloc(BUFFER_SIZE);
    if (msg != NULL) {
        memset(msg, 0, BUFFER_SIZE);
        int size = ksceNetRecvfrom(
                client_sock, msg, BUFFER_SIZE, 0x1000, NULL, 0);
        printf("ksceNetRecvfrom: sock: %i, size: %i (0x%08x), msg: %s\n",
               client_sock, size, size, msg);
        kfree(msg);
    } else {
        printf("kmalloc failed\n");
    }

    cleanup();
    ksceKernelExitDeleteThread(0);

    return 0;
}

int module_start(SceSize argc, const void *args) {
    cmd_thid = ksceKernelCreateThread("knet", cmd_thread, 64, 0x2000, 0, 0, 0);
    if (cmd_thid >= 0) {
        ksceKernelStartThread(cmd_thid, 0, NULL);
    }

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
    return SCE_KERNEL_STOP_SUCCESS;
}

void _start() __attribute__ ((weak, alias ("module_start")));