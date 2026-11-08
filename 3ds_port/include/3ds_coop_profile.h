#ifndef CTR_COOP_PROFILE_H
#define CTR_COOP_PROFILE_H

#include <stdbool.h>
#include <stdint.h>

#define CTR_COOP_PASSWORD_LENGTH 4
#define CTR_COOP_GUEST_NAME_LENGTH 7
#define CTR_COOP_DEFAULT_GRAPHICS_ID 100

struct CtrCoopProfile
{
    uint8_t hostPassword[CTR_COOP_PASSWORD_LENGTH];
    uint8_t guestPassword[CTR_COOP_PASSWORD_LENGTH];
    uint8_t hostGraphicsId;
    uint8_t guestGraphicsId;
    uint8_t guestNameLength;
    uint8_t guestName[CTR_COOP_GUEST_NAME_LENGTH + 1];
};

void CtrCoopProfile_Load(void);
void CtrCoopProfile_Reload(void);
bool CtrCoopProfile_Save(void);
const struct CtrCoopProfile *CtrCoopProfile_Get(void);
bool CtrCoopProfile_HasHostPassword(void);
bool CtrCoopProfile_HasGuestPassword(void);
void CtrCoopProfile_SetHostPassword(const uint8_t *password);
void CtrCoopProfile_SetGuestPassword(const uint8_t *password);
void CtrCoopProfile_SetHostGraphicsId(uint8_t graphicsId);
void CtrCoopProfile_SetGuestGraphicsId(uint8_t graphicsId);
void CtrCoopProfile_SetGuestName(const uint8_t *name, uint8_t length);

#endif // CTR_COOP_PROFILE_H
