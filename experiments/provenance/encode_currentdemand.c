/* Phase 13 harness: encode ISO 15118-2:2014 CurrentDemandReq/Res
 * reference profiles with libcbv2g (EVerest) and print EXI byte counts.
 * The Req-core profile mirrors ISO 15118-2:2014 Annex D.2.3 exactly and
 * must reproduce its 25-byte stream byte-for-byte (encoder validation).
 */
#include <stdio.h>
#include <string.h>
#include <cbv2g/common/exi_bitstream.h>
#include <cbv2g/iso_2/iso2_msgDefDatatypes.h>
#include <cbv2g/iso_2/iso2_msgDefEncoder.h>

#define BUF 4096

static void set_header(struct iso2_exiDocument *d) {
    /* SessionID = 3031323334353637 (Annex D examples) */
    static const uint8_t sid[8] = {0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37};
    memcpy(d->V2G_Message.Header.SessionID.bytes, sid, 8);
    d->V2G_Message.Header.SessionID.bytesLen = 8;
}

static void pv(struct iso2_PhysicalValueType *p, int8_t m, iso2_unitSymbolType u, int16_t v) {
    p->Multiplier = m; p->Unit = u; p->Value = v;
}

static size_t run(const char *name, struct iso2_exiDocument *d, uint8_t *out) {
    exi_bitstream_t s;
    exi_bitstream_init(&s, out, BUF, 0, NULL);
    int err = encode_iso2_exiDocument(&s, d);
    size_t n = exi_bitstream_get_length(&s);
    printf("%s,%zu,%d,", name, n, err);
    for (size_t i = 0; i < n; i++) printf("%02X", out[i]);
    printf("\n");
    return n;
}

int main(void) {
    uint8_t out[BUF];
    struct iso2_exiDocument doc;

    /* ---- CurrentDemandReq core (= Annex D.2.3 example values) ---- */
    memset(&doc, 0, sizeof(doc));   /* init_iso2_exiDocument is a no-op */
    set_header(&doc);
    doc.V2G_Message.Body.CurrentDemandReq_isUsed = 1;
    init_iso2_CurrentDemandReqType(&doc.V2G_Message.Body.CurrentDemandReq);
    struct iso2_CurrentDemandReqType *rq = &doc.V2G_Message.Body.CurrentDemandReq;
    rq->DC_EVStatus.EVReady = 1;
    rq->DC_EVStatus.EVErrorCode = iso2_DC_EVErrorCodeType_NO_ERROR;
    rq->DC_EVStatus.EVRESSSOC = 55;
    pv(&rq->EVTargetCurrent, 0, iso2_unitSymbolType_A, 60);
    rq->ChargingComplete = 0;
    pv(&rq->EVTargetVoltage, 0, iso2_unitSymbolType_V, 450);
    run("CurrentDemandReq,core", &doc, out);

    /* ---- CurrentDemandReq conservative (all optionals on) ---- */
    rq->EVMaximumVoltageLimit_isUsed = 1;
    pv(&rq->EVMaximumVoltageLimit, 0, iso2_unitSymbolType_V, 1000);
    rq->EVMaximumCurrentLimit_isUsed = 1;
    pv(&rq->EVMaximumCurrentLimit, 0, iso2_unitSymbolType_A, 400);
    rq->EVMaximumPowerLimit_isUsed = 1;
    pv(&rq->EVMaximumPowerLimit, 3, iso2_unitSymbolType_W, 350);
    rq->BulkChargingComplete_isUsed = 1;
    rq->BulkChargingComplete = 0;
    rq->RemainingTimeToFullSoC_isUsed = 1;
    pv(&rq->RemainingTimeToFullSoC, 0, iso2_unitSymbolType_s, 28800);
    rq->RemainingTimeToBulkSoC_isUsed = 1;
    pv(&rq->RemainingTimeToBulkSoC, 0, iso2_unitSymbolType_s, 14400);
    run("CurrentDemandReq,conservative", &doc, out);

    /* ---- CurrentDemandRes core ---- */
    memset(&doc, 0, sizeof(doc));   /* init_iso2_exiDocument is a no-op */
    set_header(&doc);
    doc.V2G_Message.Body.CurrentDemandRes_isUsed = 1;
    init_iso2_CurrentDemandResType(&doc.V2G_Message.Body.CurrentDemandRes);
    struct iso2_CurrentDemandResType *rs = &doc.V2G_Message.Body.CurrentDemandRes;
    rs->ResponseCode = iso2_responseCodeType_OK;
    rs->DC_EVSEStatus.NotificationMaxDelay = 0;
    rs->DC_EVSEStatus.EVSENotification = iso2_EVSENotificationType_None;
    rs->DC_EVSEStatus.EVSEStatusCode = iso2_DC_EVSEStatusCodeType_EVSE_Ready;
    pv(&rs->EVSEPresentVoltage, 0, iso2_unitSymbolType_V, 450);
    pv(&rs->EVSEPresentCurrent, 0, iso2_unitSymbolType_A, 60);
    rs->EVSECurrentLimitAchieved = 0;
    rs->EVSEVoltageLimitAchieved = 0;
    rs->EVSEPowerLimitAchieved = 0;
    memcpy(rs->EVSEID.characters, "FRA23E45B78C", 12);
    rs->EVSEID.charactersLen = 12;
    rs->SAScheduleTupleID = 1;
    run("CurrentDemandRes,core", &doc, out);

    /* ---- CurrentDemandRes conservative (all optionals on) ---- */
    rs->DC_EVSEStatus.EVSEIsolationStatus_isUsed = 1;
    rs->DC_EVSEStatus.EVSEIsolationStatus = iso2_isolationLevelType_Valid;
    rs->EVSEMaximumVoltageLimit_isUsed = 1;
    pv(&rs->EVSEMaximumVoltageLimit, 0, iso2_unitSymbolType_V, 1000);
    rs->EVSEMaximumCurrentLimit_isUsed = 1;
    pv(&rs->EVSEMaximumCurrentLimit, 0, iso2_unitSymbolType_A, 400);
    rs->EVSEMaximumPowerLimit_isUsed = 1;
    pv(&rs->EVSEMaximumPowerLimit, 3, iso2_unitSymbolType_W, 350);
    rs->MeterInfo_isUsed = 1;
    init_iso2_MeterInfoType(&rs->MeterInfo);
    memcpy(rs->MeterInfo.MeterID.characters, "DE0000000000000000000000000MTR01", 32);
    rs->MeterInfo.MeterID.charactersLen = 32;
    rs->MeterInfo.MeterReading_isUsed = 1;
    rs->MeterInfo.MeterReading = 123456789012ULL;
    rs->MeterInfo.SigMeterReading_isUsed = 1;
    memset(rs->MeterInfo.SigMeterReading.bytes, 0xAB, 64);
    rs->MeterInfo.SigMeterReading.bytesLen = 64;
    rs->MeterInfo.MeterStatus_isUsed = 1;
    rs->MeterInfo.MeterStatus = 1;
    rs->MeterInfo.TMeter_isUsed = 1;
    rs->MeterInfo.TMeter = 1774000000LL;
    rs->ReceiptRequired_isUsed = 1;
    rs->ReceiptRequired = 1;
    run("CurrentDemandRes,conservative", &doc, out);

    return 0;
}
