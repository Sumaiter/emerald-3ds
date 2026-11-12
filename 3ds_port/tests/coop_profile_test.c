#include <stdio.h>
#include <errno.h>
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
static bool sFailRename;
#define CHECK(cond, ...) do { if (!(cond)) { sFailures++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

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
    {
        errno = EIO;
        return -1;
    }
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

static void TestFailedRenameRecovery(void)
{
    static const uint8_t password[] = {1, 2, 3, 4};
    FILE *file;

    CtrCoopProfile_SetHostPassword(password);
    sFailRename = true;
    CHECK(!CtrCoopProfile_Save(), "rename failure is reported");
    sFailRename = false;
    CtrCoopProfile_Reload();
    CHECK(memcmp(CtrCoopProfile_Get()->hostPassword, password, sizeof(password)) == 0,
        "complete temporary profile survives a failed rename and reload");
    file = fopen(MAIN_PATH, "wb");
    CHECK(file != NULL, "incomplete destination can be simulated");
    if (file != NULL) fclose(file);
    CtrCoopProfile_Reload();
    CHECK(memcmp(CtrCoopProfile_Get()->hostPassword, password, sizeof(password)) == 0,
        "valid temporary profile also recovers an incomplete destination");
    CHECK(CtrCoopProfile_Save(), "recovered profile can be saved again");
}

static void TestCorruptTerminator(void)
{
    static const uint8_t password[] = {4, 3, 2, 1};
    struct DiskProfile disk = {0};
    FILE *file;

    CtrCoopProfile_SetHostPassword(password);
    ReadDisk(&disk);
    disk.profile.guestName[CTR_COOP_GUEST_NAME_LENGTH] = 0;
    file = fopen(MAIN_PATH, "wb");
    CHECK(file != NULL, "corrupt terminator can be written");
    if (file == NULL) return;
    fwrite(&disk, 1, sizeof(disk), file);
    fclose(file);
    CtrCoopProfile_Reload();
    CHECK(!CtrCoopProfile_HasHostPassword(), "checksum is checked before normalizing the name terminator");
}

int main(void)
{
    __real_remove(MAIN_PATH);
    __real_remove(TEMP_PATH);
    TestRoundtrip();
    TestCorruption();
    TestInvalidPassword();
    TestFailedRenameRecovery();
    TestCorruptTerminator();
    __real_remove(MAIN_PATH);
    __real_remove(TEMP_PATH);
    if (sFailures)
    {
        printf("%d co-op profile tests failed\n", sFailures);
        return 1;
    }
    puts("co-op profile tests passed");
    return 0;
}
