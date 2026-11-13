#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "3ds_coop_profile.h"
#include "3ds_data.h"
#include "3ds_platform.h"

#define PROFILE_FILE "coop_profile.bin"
#define PROFILE_TEMP "coop_profile.bin.tmp"
#define PROFILE_MAGIC 0x504f4f43u
#define PROFILE_VERSION 1

struct DiskProfile
{
    uint32_t magic;
    uint16_t version;
    uint16_t reserved;
    uint32_t checksum;
    struct CtrCoopProfile profile;
};

static struct CtrCoopProfile sProfile;
static bool sLoaded;

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

static void Defaults(void)
{
    memset(&sProfile, 0xFF, sizeof(sProfile));
    sProfile.hostGraphicsId = CTR_COOP_DEFAULT_GRAPHICS_ID;
    sProfile.guestGraphicsId = CTR_COOP_DEFAULT_GRAPHICS_ID;
    sProfile.guestNameLength = 0;
    memset(sProfile.guestName, 0xFF, sizeof(sProfile.guestName));
}

static bool PasswordValid(const uint8_t *password)
{
    bool unset = true, set = true;

    for (int i = 0; i < CTR_COOP_PASSWORD_LENGTH; i++)
    {
        if (password[i] != 0xFF)
            unset = false;
        if (password[i] >= 10)
            set = false;
    }
    return unset || set;
}

static void Validate(void)
{
    if (!PasswordValid(sProfile.hostPassword))
        memset(sProfile.hostPassword, 0xFF, sizeof(sProfile.hostPassword));
    if (!PasswordValid(sProfile.guestPassword))
        memset(sProfile.guestPassword, 0xFF, sizeof(sProfile.guestPassword));
    if (sProfile.guestNameLength > CTR_COOP_GUEST_NAME_LENGTH)
    {
        sProfile.guestNameLength = 0;
        memset(sProfile.guestName, 0xFF, sizeof(sProfile.guestName));
    }
    else
    {
        memset(sProfile.guestName + sProfile.guestNameLength, 0xFF,
               sizeof(sProfile.guestName) - sProfile.guestNameLength);
    }
}

void CtrCoopProfile_Reload(void)
{
    sLoaded = false;
    CtrCoopProfile_Load();
}

static bool ReadProfile(const char *name, struct DiskProfile *disk)
{
    FILE *file = CtrFs_OpenData(name, "rb");
    bool valid = false;

    if (file == NULL)
        return false;
    if (fread(disk, 1, sizeof(*disk), file) == sizeof(*disk))
        valid = disk->magic == PROFILE_MAGIC && disk->version == PROFILE_VERSION
             && disk->checksum == Checksum(&disk->profile, sizeof(disk->profile));
    fclose(file);
    return valid;
}

void CtrCoopProfile_Load(void)
{
    struct DiskProfile disk;

    if (sLoaded)
        return;
    sLoaded = true;
    Defaults();
    if (ReadProfile(PROFILE_FILE, &disk) || ReadProfile(PROFILE_TEMP, &disk))
        sProfile = disk.profile;
    Validate();
}

bool CtrCoopProfile_Save(void)
{
    struct DiskProfile disk = {0};
    FILE *file;
    bool ok;

    CtrCoopProfile_Load();
    disk.magic = PROFILE_MAGIC;
    disk.version = PROFILE_VERSION;
    disk.profile = sProfile;
    disk.checksum = Checksum(&disk.profile, sizeof(disk.profile));
    file = CtrFs_OpenData(PROFILE_TEMP, "wb");
    if (file == NULL)
        return false;
    ok = fwrite(&disk, 1, sizeof(disk), file) == sizeof(disk);
    if (fclose(file) != 0)
        ok = false;
    if (!ok)
    {
        remove(CTR_DATA_DIR PROFILE_TEMP);
        return false;
    }
    if (remove(CTR_DATA_DIR PROFILE_FILE) != 0 && errno != ENOENT)
        return false;
    /* Keep the verified temporary file if replacement fails, so Load can
     * recover it when the old destination was already removed. */
    return rename(CTR_DATA_DIR PROFILE_TEMP, CTR_DATA_DIR PROFILE_FILE) == 0;
}

const struct CtrCoopProfile *CtrCoopProfile_Get(void)
{
    CtrCoopProfile_Load();
    return &sProfile;
}

bool CtrCoopProfile_HasHostPassword(void)
{
    return CtrCoopProfile_Get()->hostPassword[0] != 0xFF;
}

bool CtrCoopProfile_HasGuestPassword(void)
{
    return CtrCoopProfile_Get()->guestPassword[0] != 0xFF;
}

void CtrCoopProfile_SetHostPassword(const uint8_t *password)
{
    CtrCoopProfile_Load();
    memcpy(sProfile.hostPassword, password, sizeof(sProfile.hostPassword));
    CtrCoopProfile_Save();
}

void CtrCoopProfile_SetGuestPassword(const uint8_t *password)
{
    CtrCoopProfile_Load();
    memcpy(sProfile.guestPassword, password, sizeof(sProfile.guestPassword));
    CtrCoopProfile_Save();
}

void CtrCoopProfile_SetHostGraphicsId(uint8_t graphicsId)
{
    CtrCoopProfile_Load();
    sProfile.hostGraphicsId = graphicsId;
    CtrCoopProfile_Save();
}

void CtrCoopProfile_SetGuestGraphicsId(uint8_t graphicsId)
{
    CtrCoopProfile_Load();
    sProfile.guestGraphicsId = graphicsId;
    CtrCoopProfile_Save();
}

void CtrCoopProfile_SetGuestName(const uint8_t *name, uint8_t length)
{
    CtrCoopProfile_Load();
    if (length > CTR_COOP_GUEST_NAME_LENGTH)
        length = CTR_COOP_GUEST_NAME_LENGTH;
    memset(sProfile.guestName, 0xFF, sizeof(sProfile.guestName));
    if (length)
        memcpy(sProfile.guestName, name, length);
    sProfile.guestNameLength = length;
    CtrCoopProfile_Save();
}
