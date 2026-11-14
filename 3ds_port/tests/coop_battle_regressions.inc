/* Included by the real session fixture: these checks exercise the backend,
 * packet codecs and complete PC recovery transactions, not a model of them. */
static void TestBattleTransactionIdentity(void)
{
    uint8_t oldAck[CTR_COOP_BATTLE_READY_SIZE], begin[CTR_COOP_BATTLE_BEGIN_SIZE];
    uint8_t payload[BATTLE_HEADER_SIZE + sizeof(gPlayerParty) + BATTLE_STORAGE_RECORD_SIZE];
    uint32_t size, checksum;
    ReadyHost();
    CHECK(CtrCoop_BattleStart(), "first transaction starts");
    CtrCoop_BattleEnd();
    CtrCoopProtocol_EncodeBattleReady(sCoop.battleSendSequence, sCoop.battleSendChecksum, oldAck, sizeof(oldAck));
    AcknowledgeBattle();
    CHECK(CtrCoop_BattleStart(), "second transaction starts");
    CtrCoop_BattleEnd();
    CtrCoop_HandlePacket(PEER_ID, oldAck, sizeof(oldAck));
    CHECK(sCoop.battleSendState == BATTLE_SYNC_SENDING, "old ACK cannot discard the new frozen result");
    CtrCoopProtocol_EncodeBattleReady(sCoop.battleSendSequence, sCoop.battleSendChecksum ^ 1, oldAck, sizeof(oldAck));
    CtrCoop_HandlePacket(PEER_ID, oldAck, sizeof(oldAck));
    CHECK(sCoop.battleSendState == BATTLE_SYNC_SENDING, "right sequence with wrong checksum cannot confirm");
    CtrCoop_HandlePacket(PEER_ID, (uint8_t[]){CTR_COOP_BATTLE_READY_PACKET, CTR_COOP_PROTOCOL_VERSION}, 2);
    CHECK(sCoop.battleSendState == BATTLE_SYNC_SENDING, "legacy unbound ACK cannot confirm");
    AcknowledgeBattle();
    CHECK(sCoop.battleSendState == BATTLE_SYNC_IDLE, "matching ACK completes only its transaction");

    ReadyHost();
    size = BuildBattlePayload(payload, 91);
    payload[14] = 1;
    ReceiveBattlePayload(payload, size);
    checksum = CtrCoopProtocol_Checksum(payload, size);
    CtrCoopProtocol_EncodeBattleBegin(size, checksum, 1, begin, sizeof(begin));
    ClearPackets();
    CtrCoop_HandlePacket(PEER_ID, begin, sizeof(begin));
    CHECK(sCoop.battleReceiveState == BATTLE_SYNC_IDLE && LastPacket(CTR_COOP_BATTLE_READY_PACKET),
        "queued completed BEGIN retries ACK without reopening its transfer");
    payload[14] = 2;
    uint8_t next[CTR_COOP_BATTLE_BEGIN_SIZE];
    CtrCoopProtocol_EncodeBattleBegin(size, CtrCoopProtocol_Checksum(payload, size), 2, next, sizeof(next));
    CtrCoop_HandlePacket(PEER_ID, next, sizeof(next));
    CtrCoop_HandlePacket(PEER_ID, begin, sizeof(begin));
    CHECK(sCoop.battleReceiveState == BATTLE_SYNC_RECEIVING && sCoop.battleReceiveSequence == 2,
        "old BEGIN leaves an ongoing new receive untouched");
    ReceiveBattlePayload(payload, size);
    CHECK(sCoop.battleAppliedSequence == 2, "new transfer finishes after queued old BEGIN");
    CtrCoop_HandlePacket(PEER_ID, begin, sizeof(begin));
    CHECK(sCoop.battleReceiveState == BATTLE_SYNC_IDLE, "older completed BEGIN cannot reopen after result 2");

    ReadyHost();
    payload[14] = 255; payload[15] = 255;
    ReceiveBattlePayload(payload, size);
    payload[14] = 0; payload[15] = 0;
    ReceiveBattlePayload(payload, size);
    CHECK(sCoop.battleAppliedSequence == 0, "sequence wrap accepts the next result");
    payload[14] = 255; payload[15] = 255;
    ReceiveBattlePayload(payload, size);
    CHECK(sCoop.battleAppliedSequence == 0 && sCoop.battleReceiveState == BATTLE_SYNC_IDLE,
        "sequence wrap rejects the old result");

    ReadyHost();
    CtrCoopProtocol_EncodeBattleBegin(size, CtrCoopProtocol_Checksum(payload, size), 17, begin, sizeof(begin));
    CtrCoop_HandlePacket(PEER_ID, begin, sizeof(begin));
    SendBattleTransfer(payload, size, CtrCoopProtocol_Checksum(payload, size), false, 0);
    CHECK(!sCoop.battleHaveApplied && sCoop.battleReceiveOffset == 0,
        "payload sequence must match the announced transaction before application");
}

static void TestSpecialTrainerFlagRelease(void)
{
    const uint32_t modes[] = {BATTLE_TYPE_SECRET_BASE, BATTLE_TYPE_BATTLE_TOWER,
        BATTLE_TYPE_DOME, BATTLE_TYPE_PALACE, BATTLE_TYPE_ARENA, BATTLE_TYPE_FACTORY,
        BATTLE_TYPE_PIKE, BATTLE_TYPE_PYRAMID, BATTLE_TYPE_TRAINER_HILL, BATTLE_TYPE_EREADER_TRAINER};
    for (unsigned role = 0; role < 2; ++role)
        for (unsigned mode = 0; mode <= sizeof(modes) / sizeof(modes[0]); ++mode)
        {
            ReadyHost();
            sCoop.guest = role;
            sCoop.authenticated = sCoop.saveReady = true;
            gBattleTypeFlags = BATTLE_TYPE_TRAINER | (mode == 0 ? 0 : modes[mode - 1]);
            gTrainerBattleOpponent_A = mode == 0 ? 1024 : 50;
            sCoop.battleTrainerId = gTrainerBattleOpponent_A;
            CHECK(BeginLocalBattle(), "special battle begins");
            gBattleOutcome = B_OUTCOME_WON;
            CtrCoop_BattleEnd();
            AcknowledgeBattle();
            CtrCoop_BattleTick();
            CHECK(!sCoop.battleReleasePending, "special win releases without ordinary flag (role %u mode %u)", role, mode);
        }
    ReadyHost();
    gBattleTypeFlags = BATTLE_TYPE_TRAINER;
    gTrainerBattleOpponent_A = 7;
    CHECK(CtrCoop_BattleStart(), "ordinary trainer starts");
    gBattleOutcome = B_OUTCOME_WON;
    CtrCoop_BattleEnd(); AcknowledgeBattle(); CtrCoop_BattleTick();
    CHECK(sCoop.battleReleasePending, "ordinary victory still awaits its story flag");
    sSaveBlock1.flags[(TRAINER_FLAGS_START + 7) / 8] |= 1 << ((TRAINER_FLAGS_START + 7) & 7);
    CtrCoop_BattleTick();
    CHECK(!sCoop.battleReleasePending, "ordinary victory releases after flag");
    ReadyHost();
    gBattleTypeFlags = BATTLE_TYPE_RECORDED | BATTLE_TYPE_TRAINER;
    CHECK(CtrCoop_BattleStart() && !sCoop.battleActive, "recorded playback creates no gameplay transaction");
    gBattleTypeFlags = BATTLE_TYPE_WALLY_TUTORIAL;
    CHECK(CtrCoop_BattleStart() && !sCoop.battleActive, "Wally tutorial does not store his temporary Pokemon as player party");
}

static void TestBattleReturnDisconnectAndReset(void)
{
    ReadyHost();
    memcpy(sSaveBlock1.playerParty, gPlayerParty, sizeof(gPlayerParty));
    sSaveBlock1.playerPartyCount = gPlayerPartyCount;
    gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_BATTLE_TOWER;
    CHECK(CtrCoop_BattleStart(), "facility transaction starts");
    gBattleOutcome = B_OUTCOME_LOST;
    CtrCoop_BattleEndAfterPartyRestore();
    ClearRemote();
    CHECK(sCoop.battleActive && sBattleReturnPending, "disconnect preserves ongoing native party restoration");
    CtrCoop_BattlePartyRestored();
    gMain.callback2 = CB2_Overworld;
    CtrCoop_BattleTick();
    CHECK(!sCoop.battleActive && !sBattleReturnPending, "offline return still finishes after full restoration");
    ReadyHost();
    CHECK(CtrCoop_BattleStart(), "second battle starts before reset");
    CtrCoop_BattleEndAfterPartyRestore();
    CtrCoop_SoftReset();
    CHECK(!sCoop.battleActive && !sBattleReturnPending && !sBattlePartyRestoreSeen,
        "soft reset discards old transaction and deferred return state");
}

static void FillCatchCollision(void)
{
    for (uint8_t box = 0; box < TOTAL_BOXES_COUNT; ++box)
        for (uint8_t pos = 0; pos < IN_BOX_COUNT; ++pos)
            if (box != CTR_COOP_STORAGE_BOX_ID || pos >= CTR_COOP_STORAGE_ROW_SIZE * 2)
                SetBoxMon(&sStorage.boxes[box][pos], 90);
}

static void TestBattlePcRecoveryGuards(void)
{
    uint8_t payload[BATTLE_HEADER_SIZE + sizeof(gPlayerParty) + BATTLE_STORAGE_RECORD_SIZE];
    uint8_t packet[CTR_COOP_PC_RECOVER_SIZE];
    uint32_t size = BuildBattlePayload(payload, 91);
    const struct SentPacket *result;
    bool accepted;
    ReadyHost();
    FillCatchCollision();
    sCoop.peerBattleActive = true;
    ReceiveBattlePayload(payload, size);
    CHECK(BattlePcRecovery() && sCoop.battleReceiveState == BATTLE_SYNC_READY,
        "full storage retains complete unapplied catch");
    CHECK(CtrCoop_RequestPC(), "receiver may open PC to free space after combat completes");
    SetBoxMon(&sStorage.boxes[0][1], 0);
    CtrCoop_BattleTick();
    CHECK(sStorage.boxes[0][1].species == 0 && !LastPacket(CTR_COOP_BATTLE_READY_PACKET),
        "pending catch cannot mutate the open PC");
    CtrCoop_CommitPC();
    CHECK(sStorage.boxes[0][1].species == 91 && LastPacket(CTR_COOP_BATTLE_READY_PACKET),
        "closing PC applies preserved catch exactly once");
    CHECK(((const struct PokemonStorage *)sSaveData)->boxes[0][1].species == 91,
        "PC transfer includes the recovered catch for convergence");

    ReadyHost();
    sCoop.guest = true; sCoop.authenticated = sCoop.saveReady = true;
    FillCatchCollision(); ReceiveBattlePayload(payload, size);
    CHECK(!CtrCoop_RequestPC() && sCoop.pcPending, "guest recovery still awaits host arbitration");
    result = LastPacket(CTR_COOP_PC_RECOVER_PACKET);
    CHECK(result != NULL, "guest identifies the blocked result in its recovery request");

    ReadyHost();
    CHECK(CtrCoop_BattleStart(), "host combat begins before recovery request");
    CtrCoop_BattleEnd();
    CtrCoopProtocol_EncodePcRecover(sCoop.battleSendSequence, sCoop.battleSendChecksum, packet, sizeof(packet));
    sCoop.peerBattleActive = true;
    CtrCoop_HandlePacket(PEER_ID, packet, sizeof(packet));
    CHECK(sCoop.pcOwner == PC_OWNER_NONE, "recovery cannot open while peer combat is uncompleted");
    sCoop.peerBattleFinished = true;
    packet[4] ^= 1;
    CtrCoop_HandlePacket(PEER_ID, packet, sizeof(packet));
    CHECK(sCoop.pcOwner == PC_OWNER_NONE, "wrong transaction cannot grant recovery");
    packet[4] ^= 1;
    sCoop.battleActive = true;
    CtrCoop_HandlePacket(PEER_ID, packet, sizeof(packet));
    CHECK(sCoop.pcOwner == PC_OWNER_NONE, "recovery cannot race active host combat");
    sCoop.battleActive = false;
    CtrCoop_HandlePacket(PEER_ID, packet, sizeof(packet));
    result = LastPacket(CTR_COOP_PC_RESULT_PACKET);
    CHECK(result && CtrCoopProtocol_DecodePcResult(result->data, result->len, &accepted) && accepted
        && sCoop.pcOwner == PC_OWNER_GUEST, "matching finished transfer permits guest recovery PC");
}

struct BattleEndpoint
{
    __typeof__(sCoop) coop;
    struct SaveBlock1 save1, baseline;
    struct SaveBlock2 save2;
    struct PokemonStorage storage, storageBaseline;
    struct Pokemon party[PARTY_SIZE];
    uint8_t send[CTR_COOP_BATTLE_DATA_MAX], receive[CTR_COOP_BATTLE_DATA_MAX];
};

static void SaveBattleEndpoint(struct BattleEndpoint *endpoint)
{
    endpoint->coop = sCoop;
    endpoint->save1 = sSaveBlock1; endpoint->save2 = sSaveBlock2;
    endpoint->baseline = sBattleSaveBaseline;
    endpoint->storage = sStorage; endpoint->storageBaseline = sBattleStorageBaseline;
    memcpy(endpoint->party, gPlayerParty, sizeof(gPlayerParty));
    memcpy(endpoint->send, sBattleSendData, sizeof(sBattleSendData));
    memcpy(endpoint->receive, sBattleReceiveData, sizeof(sBattleReceiveData));
}

static void LoadBattleEndpoint(const struct BattleEndpoint *endpoint)
{
    sCoop = endpoint->coop;
    sSaveBlock1 = endpoint->save1; sSaveBlock2 = endpoint->save2;
    sBattleSaveBaseline = endpoint->baseline;
    sStorage = endpoint->storage; sBattleStorageBaseline = endpoint->storageBaseline;
    memcpy(gPlayerParty, endpoint->party, sizeof(gPlayerParty));
    memcpy(sBattleSendData, endpoint->send, sizeof(sBattleSendData));
    memcpy(sBattleReceiveData, endpoint->receive, sizeof(sBattleReceiveData));
    CtrCoop_ResetBattleReturn();
    ClearPackets();
}

static void TestTwoPeerFullStorageRecovery(void)
{
    static struct BattleEndpoint host, guest;
    static uint8_t pcData[CTR_COOP_PC_SYNC_SIZE];
    uint8_t guestAck[CTR_COOP_BATTLE_READY_SIZE];
    for (unsigned action = 0; action < 6; ++action)
    for (unsigned recoveryGuest = 0; recoveryGuest < 2; ++recoveryGuest)
    {
        ReadyHost(); FillCatchCollision(); SetBoxMon(&sStorage.boxes[0][0], 0);
        SetBattleMoney(100);
        sCoop.peerBattleActive = true;
        CHECK(CtrCoop_BattleStart(), "host starts independent catch");
        SetBoxMon(&sStorage.boxes[0][0], 92);
        SetBattleMoney(105);
        sSaveBlock1.bagPocket_Items[0] = (struct ItemSlot){13, 1};
        CtrCoop_BattleEnd(); SaveBattleEndpoint(&host);
        ReadyHost(); sCoop.guest = true; sCoop.authenticated = sCoop.saveReady = true;
        FillCatchCollision(); SetBoxMon(&sStorage.boxes[0][0], 0);
        SetBattleMoney(100);
        CHECK(BeginLocalBattle(), "guest starts independent catch");
        SetBoxMon(&sStorage.boxes[0][0], 91);
        SetBattleMoney(107);
        sSaveBlock1.bagPocket_Items[0] = (struct ItemSlot){13, 1};
        CtrCoop_BattleEnd();
        ReceiveBattlePayload(host.send, host.coop.battleSendSize);
        CHECK(BattlePcRecovery(), "guest collision retains host result");
        SaveBattleEndpoint(&guest);
        LoadBattleEndpoint(&host);
        ReceiveBattlePayload(guest.send, guest.coop.battleSendSize);
        CHECK(BattlePcRecovery(), "host collision retains guest result");
        SaveBattleEndpoint(&host);

        if (recoveryGuest)
        {
            LoadBattleEndpoint(&guest);
            CtrCoop_RequestPC();
            struct SentPacket recovery = *LastPacket(CTR_COOP_PC_RECOVER_PACKET);
            SaveBattleEndpoint(&guest);
            LoadBattleEndpoint(&host);
            CtrCoop_HandlePacket(PEER_ID, recovery.data, recovery.len);
            CHECK(sCoop.pcOwner == PC_OWNER_GUEST, "host arbitrates recovery during both pending results");
            struct SentPacket grant = *LastPacket(CTR_COOP_PC_RESULT_PACKET);
            SaveBattleEndpoint(&host);
            LoadBattleEndpoint(&guest);
            CtrCoop_HandlePacket(PEER_ID, grant.data, grant.len);
            CHECK(CtrCoop_RequestPC(), "guest opens only after matching host grant");
        }
        else
        {
            CHECK(CtrCoop_RequestPC(), "host opens recovery during both pending results");
            struct SentPacket claim = *LastPacket(CTR_COOP_PC_CLAIM_PACKET);
            SaveBattleEndpoint(&host);
            LoadBattleEndpoint(&guest);
            CtrCoop_HandlePacket(PEER_ID, claim.data, claim.len);
            CHECK(sCoop.pcOwner == PC_OWNER_HOST, "guest respects recovery lease");
            SaveBattleEndpoint(&guest);
            LoadBattleEndpoint(&host);
        }
        if (action == 0)
            SetBoxMon(&sStorage.boxes[0][1], 0);
        else if (action <= 2 || action == 5)
        {
            sStorage.boxes[0][1] = sStorage.boxes[0][0];
            SetBoxMon(&sStorage.boxes[0][0], 0);
            if (action == 2)
                sStorage.boxes[0][1].data[8] = 0xAA; /* subsequent held-item/name edit */
        }
        else if (action == 3)
            SetBoxMon(&sStorage.boxes[0][0], 0); /* intentionally release own new catch */
        else
        {
            sStorage.boxes[0][1] = gPlayerParty[0].box;
            gPlayerParty[0].box = sStorage.boxes[0][0];
            SetBoxMon(&sStorage.boxes[0][0], 0); /* withdraw own catch into party */
        }
        CtrCoop_CommitPC();
        CHECK(sCoop.syncState == PC_SYNC_SENDING && !sCoop.battleStorageBlocked,
            "PC close merges pending catch then freezes storage for peer");
        memcpy(pcData, sSaveData, sizeof(pcData));
        memcpy(guestAck, LastPacket(CTR_COOP_BATTLE_READY_PACKET)->data, sizeof(guestAck));
        SaveBattleEndpoint(recoveryGuest ? &guest : &host);
        LoadBattleEndpoint(recoveryGuest ? &host : &guest);
        CtrCoop_HandlePacket(PEER_ID, guestAck, sizeof(guestAck));
        SendTransfer(CtrCoopProtocol_EncodePcSyncBegin, CtrCoopProtocol_EncodePcSyncChunk,
            pcData, sizeof(pcData), CtrCoopProtocol_Checksum(pcData, sizeof(pcData)), true, 0);
        CHECK(LastPacket(CTR_COOP_PC_SYNC_READY_PACKET) != NULL, "other role receives complete recovered PC snapshot");
        CtrCoop_HandlePacket(PEER_ID, (uint8_t[]){CTR_COOP_PC_RELEASE_PACKET, CTR_COOP_PROTOCOL_VERSION}, 2);
        if (action == 5)
            sFailBagChange = true;
        CtrCoop_BattleTick();
        if (action == 5)
        {
            CHECK(!LastPacket(CTR_COOP_BATTLE_READY_PACKET), "failed inventory merge withholds acknowledgement");
            sFailBagChange = false;
            const struct BattleEndpoint *owner = recoveryGuest ? &guest : &host;
            ReceiveBattlePayload(owner->send, owner->coop.battleSendSize);
        }
        CHECK(sCoop.battleReceiveState == BATTLE_SYNC_IDLE && !sCoop.battleStorageBlocked,
            "remaining battle result applies after synchronized PC release");
        const struct SentPacket *ack = LastPacket(CTR_COOP_BATTLE_READY_PACKET);
        CHECK(ack != NULL, "remaining peer result acknowledged");
        if (ack) memcpy(guestAck, ack->data, sizeof(guestAck));
        SaveBattleEndpoint(recoveryGuest ? &host : &guest);
        LoadBattleEndpoint(recoveryGuest ? &guest : &host);
        CtrCoop_HandlePacket(PEER_ID, (uint8_t[]){CTR_COOP_PC_SYNC_READY_PACKET, CTR_COOP_PROTOCOL_VERSION}, 2);
        CtrCoop_HandlePacket(PEER_ID, guestAck, sizeof(guestAck));
        CtrCoop_BattleTick();
        SaveBattleEndpoint(recoveryGuest ? &guest : &host);
        CHECK(memcmp(&host.storage, &guest.storage, sizeof(host.storage)) == 0,
            "both roles converge to identical complete storage after recovery (owner %u)", recoveryGuest);
        CHECK(host.coop.battleSendState == BATTLE_SYNC_IDLE && guest.coop.battleSendState == BATTLE_SYNC_IDLE,
            "both frozen results acknowledged after recovery");
        unsigned ownCount = 0, peerCount = 0;
        for (unsigned box = 0; box < TOTAL_BOXES_COUNT; ++box)
            for (unsigned pos = 0; pos < IN_BOX_COUNT; ++pos)
            {
                ownCount += host.storage.boxes[box][pos].species == (recoveryGuest ? 91 : 92);
                peerCount += host.storage.boxes[box][pos].species == (recoveryGuest ? 92 : 91);
            }
        CHECK(ownCount == (action == 3 ? 0u : 1u) && peerCount == 1,
            "PC edits cannot duplicate or resurrect catch (owner %u action %u)", recoveryGuest, action);
        if (action == 2)
            CHECK(host.storage.boxes[0][1].data[8] == 0xAA, "subsequent PC item/name edit is preserved");
        if (action == 4)
            CHECK((recoveryGuest ? guest.party[0] : host.party[0]).box.species == (recoveryGuest ? 91 : 92),
                "withdrawn catch remains in the owner's party");
        CHECK((host.save1.money ^ host.save2.encryptionKey) == 112
            && (guest.save1.money ^ guest.save2.encryptionKey) == 112,
            "superseding storage still applies each separate monetary reward once");
        CHECK(host.save1.bagPocket_Items[0].quantity == 2 && guest.save1.bagPocket_Items[0].quantity == 2,
            "inventory rewards still apply once including a failed merge retry");
    }
}
