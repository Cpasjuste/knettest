#include <vitasdkkern.h>
#include <libk/string.h>
#include <libk/stdbool.h>
#include <libk/stdio.h>
#include "utils.h"

#define SERVER_PORT 4444
#define BUFFER_SIZE 1024*1024
#define MAX_CHAR 256

void _start() __attribute__ ((weak, alias ("module_start")));

SceNetSockaddrIn client;
unsigned int client_size;
int server_sock = -1, client_sock = -1;
SceUID cmd_thid = -1;
bool quit = false;

int get_sock(int sock) {

    //SceNetSockaddrIn client;
    memset(&client, 0, sizeof(client));
    client.sin_len = sizeof(client);
    client_size = sizeof(client);
    return ksceNetAccept(sock, (SceNetSockaddr *) &client, &client_size);
}

int bind_port(int port) {

    SceNetSockaddrIn serverAddress;

    // prepare the sockaddr structure
    serverAddress.sin_len = sizeof(serverAddress);
    serverAddress.sin_family = SCE_NET_AF_INET;
    serverAddress.sin_addr.s_addr = SCE_NET_INADDR_ANY;
    serverAddress.sin_port = ksceNetHtons((unsigned short) port);
    memset(serverAddress.sin_zero, 0, sizeof(serverAddress.sin_zero));

    // create server socket
    int sock = ksceNetSocket("socktest",
                             SCE_NET_AF_INET,
                             SCE_NET_SOCK_STREAM, 0);

    // bind
    if (ksceNetBind(sock, (SceNetSockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
        LOG("sceNetBind failed\n");
        return -1;
    }

    // listen
    if (ksceNetListen(sock, 128) < 0) {
        LOG("sceNetListen failed\n");
        return -1;
    }

    return sock;
}

void cleanup() {

    if (client_sock >= 0) {
        ksceNetSocketClose(client_sock);
    }

    if (server_sock >= 0) {
        ksceNetSocketClose(server_sock);
    }

    kpool_free();
}

int cmd_thread(SceSize args, void *argp) {


    char *msg = kpool_alloc(BUFFER_SIZE);
    if (msg == NULL) {
        LOG("msg buffer == NULL\n");
        cleanup();
        ksceKernelExitDeleteThread(0);
        return 0;
    }

    server_sock = bind_port(SERVER_PORT);
    if (server_sock <= 0) {
        LOG("bind_port failed: %i\n", server_sock);
        return 0;
    }

    //LOG("get_sock\n");
    client_sock = get_sock(server_sock);
    if (client_sock <= 0) {
        LOG("get_sock failed: %i\n", client_sock);
        ksceNetSocketClose(server_sock);
        return 0;
    }

    //char msg[128];

    while (!quit) {

        memset(msg, 0, BUFFER_SIZE);

        int size = ksceNetRecvfrom(
                client_sock, msg, BUFFER_SIZE, 0x1000, (SceNetSockaddr *) &client, &client_size);

        if (size < 0) {
            char str[512];
            snprintf(str, 512, "ksceNetRecvfrom(%i): %i (0x%08X) : %s\n", client_sock, size, size, msg);
            LOG(str);
            ksceNetSendto(client_sock, str, strlen(str), 0, NULL, 0);
            break;
        } else {
            char str[512];
            snprintf(str, 512, "ksceNetRecvfrom(%i): %i (0x%08X) : %s\n", client_sock, size, size, msg);
            ksceNetSendto(client_sock, str, strlen(str), 0, NULL, 0);
        }
    }

    LOG("closing connection\n");

    cleanup();

    ksceKernelExitDeleteThread(0);

    return 0;
}

int module_start(SceSize argc, const void *args) {

    //LOG("module_start\n");

    cmd_thid = ksceKernelCreateThread("nettest_th", cmd_thread, 64, 0x4000, 0, 0x10000, 0);
    if (cmd_thid >= 0)
        ksceKernelStartThread(cmd_thid, 0, NULL);

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {

    quit = true;

    ksceKernelWaitThreadEnd(cmd_thid, NULL, NULL);

    return SCE_KERNEL_STOP_SUCCESS;
}
