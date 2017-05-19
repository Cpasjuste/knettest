#include <stdio.h>
#include <stdlib.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/sysmodule.h>
#include <psp2/net/netctl.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/clib.h>

#include <taihen.h>

#include "debugScreen.h"

#define printf psvDebugScreenPrintf
#define SKPRX "ux0:tai/nettest.skprx"

int exitTimeout(SceUInt delay) {

    sceKernelDelayThread(delay * 1000 * 1000);
    sceKernelExitProcess(0);

    return 0;
}

void netInit() {

    sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
    SceNetInitParam netInitParam;
    size_t size = 1 * 1024 * 1024;
    netInitParam.memory = malloc(size);
    netInitParam.size = size;
    netInitParam.flags = 0;
    sceNetInit(&netInitParam);
    sceNetCtlInit();
}

SceUID getModuleUID() {
    tai_module_info_t info;
    info.size = sizeof(info);
    if (taiGetModuleInfo("nettest", &info) >= 0) {
        return info.modid;
    }
    return -1;
}

void load() {

    if(getModuleUID() >= 0) {
        printf("nettest module already loaded\n");
        return;
    }

    //SceUID pid = sceKernelGetProcessId();
    //printf("loading %s (pid=%i)\n", SKPRX, pid);
    //int ret = taiLoadStartModuleForPid(pid, SKPRX, 0, NULL, 0);//taiLoadStartKernelModule(SKPRX, 0, NULL, 0);

    printf("loading %s\n", SKPRX);
    int ret = taiLoadStartKernelModule(SKPRX, 0, NULL, 0);
    //int ret = taiLoadKernelModule(SKPRX, 0, NULL);
    if (ret >= 0) {
        printf("nettest module loaded\n");
        SceNetCtlInfo netInfo;
        sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS, &netInfo);
        printf("telnet to %s:4444\n", netInfo.ip_address);

    } else {
        printf("could not load nettest: 0x%08x\n", ret);
    }

    sceKernelDelayThread(1000*1000);
}

void unload() {

    printf("unloading nettest\n");

    SceUID uid = getModuleUID();
    if(uid >= 0) {
        int ret = taiStopUnloadKernelModule(uid, 0, NULL, 0, NULL, NULL);
        //int ret = sceKernelStopUnloadModule(uid, 0, NULL, 0, NULL, 0);
        if (ret >= 0) {
            printf("nettest module unloaded\n");
        } else {
            printf("could not unload nettest: %i\n", ret);
        }
    } else {
        printf("nettest module not loaded\n");
    }

    sceKernelDelayThread(1000*500);
}

int main(int argc, char *argv[]) {

    SceCtrlData ctrl;

    psvDebugScreenInit();
    netInit();

    printf("nettest LOADER @ Cpasjuste\n\n");
    printf("Triangle to load nettest module\n");
    printf("Square to unload nettest module\n");
    printf("Circle to test sceClibPrintf hook\n");
    printf("Cross/Circle to exit\n");

    while (1) {

        sceCtrlPeekBufferPositive(0, &ctrl, 1);
        if (ctrl.buttons == (SCE_CTRL_CIRCLE | SCE_CTRL_CROSS))
            break;
        else if (ctrl.buttons == SCE_CTRL_TRIANGLE)
            load();
        else if (ctrl.buttons == SCE_CTRL_SQUARE)
            unload();
        else if (ctrl.buttons == SCE_CTRL_CIRCLE) {
            sceClibPrintf("Hello Module1\n");
            printf("Hello Module2\n");
            fprintf(stdout, "Hello Module3\n");
        }
    }

    return exitTimeout(0);
}
