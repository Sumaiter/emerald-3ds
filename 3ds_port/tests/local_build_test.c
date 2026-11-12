#include <stdio.h>
#include <string.h>
#define __asm__(...)
#include "3ds_local.h"
#include "3ds_log.h"
#ifndef LOCAL_BUILD_SOURCE
#define LOCAL_BUILD_SOURCE "../src/3ds_local.c"
#endif
#include LOCAL_BUILD_SOURCE

static int sFailures;
static struct AppData sAppData;
#define CHECK(cond, ...) do { if (!(cond)) { sFailures++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

void CtrLog_Write(CtrLogCategory category, const char *format, ...)
{
    (void)category;
    (void)format;
}

void LightLock_Init(LightLock *lock) { (void)lock; }
void LightLock_Lock(LightLock *lock) { (void)lock; }
void LightLock_Unlock(LightLock *lock) { (void)lock; }

Result udsGetNetworkStructApplicationData(const udsNetworkStruct *network, void *buf, size_t size, size_t *actual_size)
{
    (void)network;
    memcpy(buf, &sAppData, size < sizeof(sAppData) ? size : sizeof(sAppData));
    if (actual_size)
        *actual_size = size < sizeof(sAppData) ? size : sizeof(sAppData);
    return 0;
}

static void Prepare(struct AppData *app, udsNetworkScanInfo *net, uint8_t kind)
{
    memset(app, 0, sizeof(*app));
    memcpy(app->magic, kMagic, sizeof(app->magic));
    app->version = PROTOCOL_VERSION;
    app->kind = kind;
    app->token = 0x12345678;
    memcpy(app->build, sBuildId, sizeof(app->build));
    memcpy(app->hostName, "HOST", 4);
    memset(net, 0, sizeof(*net));
    net->network.total_nodes = 1;
    net->network.max_nodes = 2;
    sAppData = *app;
}

int main(void)
{
    struct AppData app;
    udsNetworkScanInfo net;
    uint32_t token = 0;
    uint8_t hostName[8] = {0};

    memset(sBuildId, 0x5A, sizeof(sBuildId));
    Prepare(&app, &net, LOCAL_KIND_COOP);
    CHECK(SessionCheck(&net, LOCAL_KIND_COOP, &token, hostName) == SESSION_YES, "matching build and kind accepted");
    CHECK(token == 0x12345678, "session token preserved");
    CHECK(memcmp(hostName, "HOST", 4) == 0, "host name preserved");

    app.version++;
    sAppData.version++;
    CHECK(SessionCheck(&net, LOCAL_KIND_COOP, NULL, NULL) == SESSION_OTHER, "other protocol rejected");

    Prepare(&app, &net, LOCAL_KIND_COOP);
    app.build[0]++;
    sAppData.build[0]++;
    CHECK(SessionCheck(&net, LOCAL_KIND_COOP, NULL, NULL) == SESSION_OTHER, "other build rejected");

    Prepare(&app, &net, LOCAL_KIND_UNION_ROOM);
    CHECK(SessionCheck(&net, LOCAL_KIND_COOP, NULL, NULL) == SESSION_NO, "other counter rejected");

    Prepare(&app, &net, LOCAL_KIND_COOP);
    app.magic[0]++;
    sAppData.magic[0]++;
    CHECK(SessionCheck(&net, LOCAL_KIND_COOP, NULL, NULL) == SESSION_NO, "invalid magic rejected");

    Prepare(&app, &net, LOCAL_KIND_COOP);
    net.network.total_nodes = 2;
    CHECK(SessionCheck(&net, LOCAL_KIND_COOP, NULL, NULL) == SESSION_NO, "third player refused in full two-player session");

    if (sFailures)
    {
        printf("%d local build compatibility tests failed\n", sFailures);
        return 1;
    }
    puts("local build compatibility tests passed");
    return 0;
}
