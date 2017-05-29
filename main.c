#include <vitasdkkern.h>
#include <libk/string.h>
#include <libk/stdio.h>

#define LOG_FILE "ux0:tai/nettest.log"
#define MAX_CHAR 256
#define SERVER_PORT 4444

void _start() __attribute__ ((weak, alias ("module_start")));

void log_write(const char *buffer) {
    SceUID fd = ksceIoOpen(LOG_FILE,
                           SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 6);
    if (fd < 0)
        return;

    ksceIoWrite(fd, buffer, strlen(buffer));
    ksceIoClose(fd);
}

#define LOG(...) \
    do { \
        char buffer[256]; \
        snprintf(buffer, sizeof(buffer), ##__VA_ARGS__); \
        log_write(buffer); \
    } while (0)

int get_sock(int sock) {

    SceNetSockaddrIn client;
    memset(&client, 0, sizeof(client));
    client.sin_len = sizeof(client);
    unsigned int sin_size = sizeof(client);
    return ksceNetAccept(sock, (SceNetSockaddr *) &client, &sin_size);
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

int module_start(SceSize argc, const void *args) {

    //log_reset();
    //LOG("module_start\n");

    uint32_t state;
    ENTER_SYSCALL(state);

    int server_sock = bind_port(SERVER_PORT);
    if (server_sock <= 0) {
        LOG("bind_port failed: %i\n", server_sock);
        return SCE_KERNEL_START_FAILED;
    }

    //LOG("get_sock\n");
    int client_sock = get_sock(server_sock);
    if (client_sock <= 0) {
        LOG("get_sock failed: %i\n", client_sock);
        ksceNetSocketClose(server_sock);
        return SCE_KERNEL_START_FAILED;
    }

    char msg[16];
    memset(msg, 0, 16);
    int size = ksceNetRecvfrom(client_sock, msg, 16, 0, NULL, 0);
    LOG("ksceNetRecvfrom: %i (0x%08X) : %s\n", size, size, msg);

    ksceNetSendto(client_sock, "hello\n", 6, 0, NULL, 0);

    ksceNetSocketClose(client_sock);
    ksceNetSocketClose(server_sock);

    EXIT_SYSCALL(state);

    return SCE_KERNEL_START_NO_RESIDENT;
}

int module_stop(SceSize argc, const void *args) {

    return SCE_KERNEL_STOP_SUCCESS;
}
