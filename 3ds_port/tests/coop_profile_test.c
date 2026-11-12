#include <stdio.h>
#include <string.h>
#include "3ds_coop_profile.h"

#define MAIN_PATH "build/coop_profile_test_main.bin"
#define TEMP_PATH "build/coop_profile_test_temp.bin"

struct DiskProfile
{
    uint32_t magic;
    uint16_t version;
    uint16_t reserved;
    uint32_t checksum;
    struct CtrCoopProfile profile;
};

static int sFailures;
static int sExpectedFailures;
static bool sFailRename;
#define CHECK(cond, ...) do { if (!(cond)) { sFailures++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)
#define XFAIL_CHECK(cond, ...) do { if (!(cond)) { sExpectedFailures++; printf("XFAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } else { sFailures++; printf("XPASS %s:%d: remove expected-failure marker: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

int __real_remove(const char *path);
int __real_rename(const char *from, const char *to);

static const char *MapPath(const char *path)
{
    if (strstr(path, "coop_profile.bin.tmp") != NULL)
        return TEMP_PATH;
    return MAIN_PATH;
}

FILE *CtrFs_OpenData(const char *name, const char *mode)
{
    return fopen(MapPath(name), mode);
}

int __wrap_remove(const char *path)
{
    return __real_remove(MapPath(path));
}

int __wrap_rename(const char *from, const char *to)
{
    (void)to;
    if (sFailRename)
        return -1;
    return __real_rename(MapPath(from), MAIN_PATH);
}

static uint32_t Checksum(const void *data, size_t size)
{
    const uint8_t *bytes = data;
    uint32_t hash = 2166136261u;

    for (size_t i = 0; i < size; i++)
    {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    return hash;
}

static void ReadDisk(struct DiskProfile *disk)
{
    FILE *file = fopen(MAIN_PATH, "rb");

    CHECK(file != NULL, "profile file exists");
    if (file == NULL)
        return;
    CHECK(fread(disk, 1, sizeof(*disk), file) == sizeof(*disk), "complete profile written");
    fclose(file);
}

static void TestRoundtrip(void)
{
    static const uint8_t hostPassword[] = {0, 1, 2, 3};
    static const uint8_t guestPassword[] = {4, 5, 6, 7};
    static const uint8_t guestName[] = {'G', 'A', 'S', 'T', 0xFF};
    const struct CtrCoopProfile *profile;
    struct DiskProfile disk = {0};

    profile = CtrCoopProfile_Get();
    CHECK(!CtrCoopProfile_HasHostPassword(), "host password starts unset");
    CHECK(!CtrCoopProfile_HasGuestPassword(), "guest password starts unset");
    CtrCoopProfile_SetHostPassword(hostPassword);
    CtrCoopProfile_SetGuestPassword(guestPassword);
    CtrCoopProfile_SetHostGraphicsId(100);
    CtrCoopProfile_SetGuestGraphicsId(105);
    CtrCoopProfile_SetGuestName(guestName, 4);
    ReadDisk(&disk);
    CHECK(disk.magic == 0x504f4f43u && disk.version == 1, "magic and version persisted");
    CHECK(disk.checksum == Checksum(&disk.profile, sizeof(disk.profile)), "checksum persisted");
    CHECK(memcmp(disk.profile.hostPassword, hostPassword, 4) == 0, "host password persisted");
    CHECK(memcmp(disk.profile.guestPassword, guestPassword, 4) == 0, "guest password persisted");
    CHECK(disk.profile.guestGraphicsId == 105, "guest character persisted");
    CHECK(disk.profile.guestNameLength == 4 && memcmp(disk.profile.guestName, guestName, 4) == 0, "guest name persisted");
    CtrCoopProfile_Reload();
    profile = CtrCoopProfile_Get();
    CHECK(profile->guestGraphicsId == 105 && profile->guestNameLength == 4, "profile reload preserves values");
}

static void TestCorruption(void)
{
    struct DiskProfile disk = {0};

    ReadDisk(&disk);
    disk.profile.hostPassword[0]++;
    FILE *file = fopen(MAIN_PATH, "wb");
    CHECK(file != NULL, "corrupt profile can be written");
    if (file != NULL)
    {
        fwrite(&disk, 1, sizeof(disk), file);
        fclose(file);
    }
    CtrCoopProfile_Reload();
    CHECK(!CtrCoopProfile_HasHostPassword(), "bad checksum rejected");
    CHECK(!CtrCoopProfile_HasGuestPassword(), "bad checksum resets complete profile");
    CHECK(CtrCoopProfile_Get()->guestGraphicsId == CTR_COOP_DEFAULT_GRAPHICS_ID, "bad checksum restores default character");
}

static void TestInvalidPassword(void)
{
    struct DiskProfile disk = {0};
    FILE *file;

    disk.magic = 0x504f4f43u;
    disk.version = 1;
    memset(disk.profile.hostPassword, 0xFF, 4);
    memset(disk.profile.guestPassword, 0xFF, 4);
    memset(disk.profile.guestName, 0xFF, sizeof(disk.profile.guestName));
    disk.profile.hostPassword[0] = 99;
    disk.profile.hostGraphicsId = 100;
    disk.profile.guestGraphicsId = 105;
    disk.checksum = Checksum(&disk.profile, sizeof(disk.profile));
    file = fopen(MAIN_PATH, "wb");
    CHECK(file != NULL, "invalid profile can be written");
    if (file != NULL)
    {
        fwrite(&disk, 1, sizeof(disk), file);
        fclose(file);
    }
    CtrCoopProfile_Reload();
    CHECK(!CtrCoopProfile_HasHostPassword(), "invalid host password discarded");
    CHECK(CtrCoopProfile_Get()->guestGraphicsId == 105, "valid fields survive invalid password (got %u)", CtrCoopProfile_Get()->guestGraphicsId);
}

static void TestRenameFailure(void)
{
    struct DiskProfile before;
    static const uint8_t replacement[] = {'N', 'E', 'U'};
    FILE *file;

    ReadDisk(&before);
    sFailRename = true;
    CtrCoopProfile_SetGuestName(replacement, 3);
    sFailRename = false;
    CtrCoopProfile_Reload();
    XFAIL_CHECK(CtrCoopProfile_Get()->guestNameLength == 4
        && memcmp(CtrCoopProfile_Get()->guestName, "GAST", 4) == 0,
        "Sprint 4: a failed profile rename preserves the previous complete file");

    file = fopen(MAIN_PATH, "wb");
    CHECK(file != NULL, "profile fixture can be restored");
    if (file != NULL)
    {
        CHECK(fwrite(&before, 1, sizeof(before), file) == sizeof(before), "profile fixture restored completely");
        fclose(file);
    }
    CtrCoopProfile_Reload();
}

static void TestNameLimitAndTemporaryRecovery(void)
{
    static const uint8_t longName[] = {'L', 'A', 'N', 'G', 'N', 'A', 'M', 'E', 'X'};
    struct DiskProfile disk;
    FILE *file;

    CtrCoopProfile_SetGuestName(longName, sizeof(longName));
    CtrCoopProfile_Reload();
    CHECK(CtrCoopProfile_Get()->guestNameLength == CTR_COOP_GUEST_NAME_LENGTH, "guest name is limited to protocol length");
    CHECK(CtrCoopProfile_Get()->guestName[CTR_COOP_GUEST_NAME_LENGTH] == 0xFF, "guest name padding is normalized");
    ReadDisk(&disk);
    __real_remove(MAIN_PATH);
    file = fopen(TEMP_PATH, "wb");
    CHECK(file != NULL, "temporary profile fixture can be written");
    if (file != NULL)
    {
        fwrite(&disk, 1, sizeof(disk), file);
        fclose(file);
    }
    CtrCoopProfile_Reload();
    CHECK(CtrCoopProfile_Get()->guestNameLength == CTR_COOP_GUEST_NAME_LENGTH, "complete temporary profile recovers after interrupted rename");
    CHECK(__real_rename(TEMP_PATH, MAIN_PATH) == 0, "profile fixture moved back to main path");
}

int main(void)
{
    __real_remove(MAIN_PATH);
    __real_remove(TEMP_PATH);
    TestRoundtrip();
    TestRenameFailure();
    TestNameLimitAndTemporaryRecovery();
    TestCorruption();
    TestInvalidPassword();
    __real_remove(MAIN_PATH);
    __real_remove(TEMP_PATH);
    if (sFailures)
    {
        printf("%d co-op profile tests failed\n", sFailures);
        return 1;
    }
    printf("co-op profile tests passed (%d expected acceptance failure%s)\n", sExpectedFailures, sExpectedFailures == 1 ? "" : "s");
    return 0;
}
