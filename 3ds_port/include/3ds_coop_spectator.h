#ifndef CTR_COOP_SPECTATOR_H
#define CTR_COOP_SPECTATOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CTR_SPECTATOR_WIDTH 400u
#define CTR_SPECTATOR_HEIGHT 240u
#define CTR_SPECTATOR_PIXELS (CTR_SPECTATOR_WIDTH * CTR_SPECTATOR_HEIGHT)
#define CTR_SPECTATOR_ENCODED_MAX (CTR_SPECTATOR_PIXELS + (CTR_SPECTATOR_PIXELS + 127u) / 128u)
#define CTR_SPECTATOR_CONTROL 70u
#define CTR_SPECTATOR_FRAME 71u
#define CTR_SPECTATOR_PULL 72u
#define CTR_SPECTATOR_CHUNK 73u
#define CTR_SPECTATOR_ACK 74u
#define CTR_SPECTATOR_CHUNK_BYTES 146u /* 160-byte transport minus chunk header. */
#define CTR_SPECTATOR_WINDOW 32u
#define CTR_SPECTATOR_TICK_PACKETS 8u

/* Main thread only. Call SetState/Tick in every game mode. Dispatch packets
 * only from the authenticated co-op peer. The peer battle hint is advisory;
 * a fresh, sequenced STATE announcement authorizes a spectator request. */
void CtrCoopSpectator_SetState(uint16_t peerId, bool connected, bool peerSingleBattle,
                             bool localSingleBattle, bool canWatch);
bool CtrCoopSpectator_Request(void);
void CtrCoopSpectator_Stop(void);
bool CtrCoopSpectator_IsWatching(void); /* Includes requesting/waiting for first image. */
bool CtrCoopSpectator_CanWatchPeer(void);
bool CtrCoopSpectator_HasViewer(void);
bool CtrCoopSpectator_HasFrame(void);
void CtrCoopSpectator_Tick(void);
bool CtrCoopSpectator_HandlePacket(uint16_t from, const uint8_t *data, uint16_t length);
void CtrCoopSpectator_Reset(void);
/* GBA/libctru B is bit 1. Consume the exit press and gate game input without
 * changing callbacks, script locks, party, save data, or the battle engine. */
bool CtrCoopSpectator_InputGate(uint16_t *held, uint16_t *pressed);

/* SDK-free lossless RGB332 XOR/LZ codec (protocol 17). No resolution loss.
 * base=NULL means a keyframe. A 16 KiB shared hash table is main-thread only. */
size_t CtrCoopSpectator_Encode(const uint8_t *pixels, const uint8_t *base, size_t count,
                             uint8_t *encoded, size_t capacity);
bool CtrCoopSpectator_Decode(const uint8_t *encoded, size_t bytes, const uint8_t *base,
                           uint8_t *pixels, size_t count);
uint32_t CtrCoopSpectator_Checksum(const uint8_t *data, size_t bytes);

#endif
