#include <libk/string.h>
#include <psp2kern/net/net.h>
#include <psp2common/types.h>
#include <psp2kern/kernel/debug.h>
#include <psp2kern/kernel/threadmgr/thread.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <taihen.h>

#define MAX_HOOKS 16
static SceUID g_hooks[MAX_HOOKS];
static tai_hook_ref_t ref_hooks[MAX_HOOKS];

#define printf ksceDebugPrintf

#define SERVER_PORT 4444
#define BUFFER_SIZE 2048

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

    //char *msg = kmalloc(BUFFER_SIZE);
    //if (msg != NULL) {
    char msg[BUFFER_SIZE];
    memset(msg, 0, BUFFER_SIZE);
    int flags = 0x1000;
    //flags &= ~(SCE_NET_MSG_USECRYPTO | SCE_NET_MSG_USESIGNATURE);
    printf("flags: 0x%08x\n", flags);
    int size = ksceNetRecvfrom(
            client_sock, msg, BUFFER_SIZE, flags, NULL, NULL);
    printf("ksceNetRecvfrom: sock: %i, size: %i (0x%08x), msg: %s\n",
           client_sock, size, size, msg);
    //kfree(msg);
    //} else {
    //    printf("kmalloc failed\n");
    // }

    cleanup();
    ksceKernelExitDeleteThread(0);

    return 0;
}

#if 0
static int
ksceNetRecvfrom_hook(int s, void *buf, unsigned int len, int flags, SceNetSockaddr *from, unsigned int *fromlen) {
    printf("ksceNetRecvfrom: %i %p %i %x %p %p\n",
           s, buf, len, flags, from, fromlen);
    return TAI_CONTINUE(int, ref_hooks[3], s, buf, len, flags, from, fromlen);
}
#endif

static int
FUN_81007a78_hook(int s, void *buf, unsigned int len, int flags, SceNetSockaddr *from, unsigned int *fromlen, int unk) {

    if (s == client_sock) {
        printf("FUN_81007a78_hook: %i %p %i 0x%x %p %p %i\n",
               s, buf, len, flags, from, fromlen, unk);
        return TAI_CONTINUE(int, ref_hooks[0], s, buf, len, flags, from, fromlen, 0);
    } else {
        return TAI_CONTINUE(int, ref_hooks[0], s, buf, len, flags, from, fromlen, unk);
    }
}

static int
FUN_8100072C_hook(int s, int unk) {

    int ret;

    if (s == client_sock) {
        ret = TAI_CONTINUE(int, ref_hooks[1], s, 2);
        printf("FUN_8100072C_hook: %i %i %i\n", s, unk, ret);
    } else {
        ret = TAI_CONTINUE(int, ref_hooks[1], s, unk);
    }

    return ret;
}

static int
FUN_8100676c_hook(int s, int *piVar2, unsigned int *fromlen, int *local_24, int unk2) {

    int ret;

    if (s == client_sock) {
        printf("FUN_8100676c_hook: %i %i\n", piVar2[4], piVar2[3]);
        //piVar2[3] = 1;
        //piVar2[4] = 0;
        //local_24[0] = 1;
        ret = TAI_CONTINUE(int, ref_hooks[2], s, piVar2, fromlen, local_24, unk2);
        printf("FUN_8100676c_hook: %i, %p, %i, %p, %i, %i (buf: %s, size: %i)\n",
               s, piVar2, fromlen, local_24, unk2, ret, piVar2[7], piVar2[8]);
    } else {
        ret = TAI_CONTINUE(int, ref_hooks[2], s, piVar2, fromlen, local_24, unk2);
    }

    return ret;
}

int module_start(SceSize argc, const void *args) {

    tai_module_info_t tai_info;
    tai_info.size = sizeof(tai_module_info_t);
    int res = taiGetModuleInfoForKernel(KERNEL_PID, "SceNetPs", &tai_info);
    printf("taiGetModuleInfoForKernel: %i\n", res);

    g_hooks[0] = taiHookFunctionOffsetForKernel(
            KERNEL_PID,
            &ref_hooks[0],
            tai_info.modid,
            0,
            0x7a78,
            1,
            FUN_81007a78_hook);
    printf("hook: FUN_81007a78: 0x%08X\n", g_hooks[0]);

    g_hooks[1] = taiHookFunctionOffsetForKernel(
            KERNEL_PID,
            &ref_hooks[1],
            tai_info.modid,
            0,
            0x72c,
            1,
            FUN_8100072C_hook);
    printf("hook: FUN_81007a78: 0x%08X\n", g_hooks[1]);

    g_hooks[2] = taiHookFunctionOffsetForKernel(
            KERNEL_PID,
            &ref_hooks[2],
            tai_info.modid,
            0,
            0x676c,
            1,
            FUN_8100676c_hook);
    printf("hook: FUN_8100676c: 0x%08X\n", g_hooks[2]);

#if 0
    g_hooks[3] = taiHookFunctionExportForKernel(
            KERNEL_PID,
            &ref_hooks[3],
            "SceNetPs",
            0xB2A5C920, // SceNetPsForDriver
            0x49B1669C, // ksceNetRecvfrom
            ksceNetRecvfrom_hook);
    //LOG("hook: _printf2: 0x%08X\n", g_hooks[3]);
#endif

    cmd_thid = ksceKernelCreateThread("knet", cmd_thread, 64, 0x2000, 0, 0, 0);
    if (cmd_thid >= 0) {
        ksceKernelStartThread(cmd_thid, 0, NULL);
    }

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
    for (int i = 0; i < MAX_HOOKS; i++) {
        if (g_hooks[i] >= 0)
            taiHookReleaseForKernel(g_hooks[i], ref_hooks[i]);
    }
    return SCE_KERNEL_STOP_SUCCESS;
}

void _start() __attribute__ ((weak, alias ("module_start")));