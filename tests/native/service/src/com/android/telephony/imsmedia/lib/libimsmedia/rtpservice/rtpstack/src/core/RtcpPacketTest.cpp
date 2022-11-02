/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <RtcpPacket.h>
#include <gtest/gtest.h>

extern RtpDt_Void addSdesItem(
        OUT RtcpConfigInfo* pobjRtcpCfgInfo, IN RtpDt_UChar* sdesName, IN RtpDt_UInt32 uiLength);

class RtcpPacketTest : public ::testing::Test
{
public:
protected:
    virtual void SetUp() override {}

    virtual void TearDown() override {}
};

/**
 * Test compound RTCP packet with one Sender-Report and SDES.
 * SR has zero reports and SDES has one CNAME item.
 */
TEST_F(RtcpPacketTest, DecodeCompoundSrSdesPacket)
{
    RtcpPacket rtcpPacket;

    /*
     * Real-time Transport Control Protocol (Sender Report)
     * 10.. .... = Version: RFC 1889 Version (2)
     * ..0. .... = Padding: False
     * ...0 0000 = Reception report count: 0
     * Packet type: Sender Report (200)
     * Length: 6 (28 bytes)
     * Sender SSRC: 0xb1c8cb02 (2982726402)
     * Timestamp, MSW: 3865027889 (0xe65fa531)
     * Timestamp, LSW: 1402021058 (0x539124c2)
     * [MSW and LSW as NTP timestamp: Jun 24, 2022 02:51:29.326433465 UTC]
     * RTP timestamp: 262533
     * Sender's packet count: 65
     * Sender's octet count: 51283
     *
     * Real-time Transport Control Protocol (Source description)
     * 10.. .... = Version: RFC 1889 Version (2)
     * ..0. .... = Padding: False
     * ...0 0001 = Source count: 1
     * Packet type: Source description (202)
     * Length: 10 (44 bytes)
     * Chunk 1, SSRC/CSRC 0xB1C8CB02
     *    Identifier: 0xb1c8cb02 (2982726402)
     *    SDES items
     *       Type: CNAME (user and domain) (1)
     *       Length: 31
     *       Text: 2600:100e:1008:af4f::1ebe:6851
     *       Type: END (0)
     */
    uint8_t bufSrSdesPacket[] = {0x80, 0xc8, 0x00, 0x06, 0xb1, 0xc8, 0xcb, 0x02, 0xe6, 0x5f, 0xa5,
            0x31, 0x53, 0x91, 0x24, 0xc2, 0x00, 0x04, 0x01, 0x85, 0x00, 0x00, 0x00, 0x41, 0x00,
            0x00, 0xc8, 0x53, 0x81, 0xca, 0x00, 0x0a, 0xb1, 0xc8, 0xcb, 0x02, 0x01, 0x1f, 0x32,
            0x36, 0x30, 0x30, 0x3a, 0x31, 0x30, 0x30, 0x65, 0x3a, 0x31, 0x30, 0x30, 0x38, 0x3a,
            0x61, 0x66, 0x34, 0x66, 0x3a, 0x3a, 0x31, 0x65, 0x62, 0x65, 0x3a, 0x36, 0x38, 0x35,
            0x31, 0x00, 0x00, 0x00, 0x00};

    RtpDt_UChar IPAddress[] = "2600:100e:1008:af4f::1ebe:6851";
    RtcpConfigInfo rtcpConfigInfo;
    addSdesItem(&rtcpConfigInfo, IPAddress, strlen((char*)IPAddress));

    RtpBuffer rtpBuffer(72, bufSrSdesPacket);
    eRTP_STATUS_CODE res = rtcpPacket.decodeRtcpPacket(&rtpBuffer, 0, &rtcpConfigInfo);
    EXPECT_EQ(res, RTP_SUCCESS);

    std::list<RtcpSrPacket*> SrList = rtcpPacket.getSrPacketList();
    EXPECT_TRUE(SrList.size() != 0);
    RtcpSrPacket* rtcpSrPacket = SrList.front();
    EXPECT_TRUE(rtcpSrPacket != NULL);
    if (rtcpSrPacket == NULL)
        return;

    RtcpHeader* pRtcpHeader = rtcpSrPacket->getRtcpHdrInfo();
    EXPECT_TRUE(pRtcpHeader != NULL);
    if (pRtcpHeader == NULL)
        return;

    EXPECT_EQ(pRtcpHeader->getVersion(), RTP_VERSION_NUM);
    EXPECT_EQ(pRtcpHeader->getPadding(), eRTP_FALSE);
    EXPECT_EQ(pRtcpHeader->getReceptionReportCount(), 0);
    EXPECT_EQ(pRtcpHeader->getPacketType(), RTCP_SR);
    EXPECT_EQ(pRtcpHeader->getLength(), 6 * RTP_WORD_SIZE);
    EXPECT_EQ(pRtcpHeader->getSsrc(), 0xb1c8cb02);

    tRTP_NTP_TIME* ntpTime = rtcpSrPacket->getNtpTime();
    EXPECT_TRUE(ntpTime != NULL);
    if (ntpTime == NULL)
        return;

    EXPECT_EQ(ntpTime->m_uiNtpHigh32Bits, 0xe65fa531);
    EXPECT_EQ(ntpTime->m_uiNtpLow32Bits, 0x539124c2);
    EXPECT_EQ(rtcpSrPacket->getRtpTimestamp(), 0x00040185);
    EXPECT_EQ(rtcpSrPacket->getSendPktCount(), 65);
    EXPECT_EQ(rtcpSrPacket->getSendOctetCount(), 0x0000c853);

    RtcpSdesPacket* pRtcpSdesPacket = rtcpPacket.getSdesPacket();
    EXPECT_TRUE(pRtcpSdesPacket != NULL);
    if (pRtcpSdesPacket == NULL)
        return;

    std::list<RtcpChunk*> pSdesChunks = pRtcpSdesPacket->getSdesChunkList();
    EXPECT_EQ(pSdesChunks.size(), 1);
    RtcpChunk* chunk = pSdesChunks.front();
    EXPECT_TRUE(chunk != NULL);
    if (chunk == NULL)
        return;

    std::list<tRTCP_SDES_ITEM*> sdesItemList = chunk->getSdesItemList();
    EXPECT_EQ(sdesItemList.size(), 1);
    tRTCP_SDES_ITEM* sdesItem = sdesItemList.front();
    EXPECT_TRUE(sdesItem != NULL);
    if (sdesItem == NULL)
        return;

    EXPECT_EQ(sdesItem->ucType, 1);
    EXPECT_EQ(sdesItem->ucLength, 31);
    EXPECT_EQ(strncmp((char*)sdesItem->pValue, (char*)"2600:100e:1008:af4f::1ebe:6851", 31), 0);

    pRtcpHeader = pRtcpSdesPacket->getRtcpHdrInfo();
    EXPECT_TRUE(pRtcpHeader != NULL);
    if (pRtcpHeader == NULL)
        return;

    EXPECT_EQ(pRtcpHeader->getVersion(), RTP_VERSION_NUM);
    EXPECT_EQ(pRtcpHeader->getPadding(), eRTP_FALSE);
    EXPECT_EQ(pRtcpHeader->getReceptionReportCount(), 1);
    EXPECT_EQ(pRtcpHeader->getPacketType(), RTCP_SDES);
    EXPECT_EQ(pRtcpHeader->getLength(), 10 * RTP_WORD_SIZE);
    EXPECT_EQ(pRtcpHeader->getSsrc(), 0xb1c8cb02);
}

/**
 * Test RTCP packet with Sender Report and Receiver Report.
 */
TEST_F(RtcpPacketTest, DecodeCompoundSrRrPacket)
{
    RtcpPacket rtcpPacket;

    /*
     *  Real-time Transport Control Protocol (Sender Report)
     *       [Stream setup by SDP (frame 1)]
     *       10.. .... = Version: RFC 1889 Version (2)
     *       ..0. .... = Padding: False
     *       ...0 0001 = Reception report count: 1
     *       Packet type: Sender Report (200)
     *       Length: 12 (52 bytes)
     *       Sender SSRC: 0xd2bd4e3e (3535621694)
     *       Timestamp, MSW: 3314714324 (0xc59286d4)
     *       Timestamp, LSW: 3874060501 (0xe6e978d5)
     *       [MSW and LSW as NTP timestamp: Jan 14, 2005 17:58:44.902000000 UTC]
     *       RTP timestamp: 320
     *       Sender's packet count: 2
     *       Sender's octet count: 320
     *       Source 1
     *           Identifier: 0xd2bd4e3e (3535621694)
     *           SSRC contents
     *               Fraction lost: 0 / 256
     *               Cumulative number of packets lost: 0
     *           Extended highest sequence number received: 131074
     *               Sequence number cycles count: 2
     *               Highest sequence number received: 2
     *           Interarrival jitter: 0
     *           Last SR timestamp: 2262099689 (0x86d4e6e9)
     *           Delay since last SR timestamp: 1 (0 milliseconds)
     *   Real-time Transport Control Protocol (Receiver Report)
     *       [Stream setup by SDP (frame 1)]
     *       10.. .... = Version: RFC 1889 Version (2)
     *       ..0. .... = Padding: False
     *       ...0 0001 = Reception report count: 1
     *       Packet type: Receiver Report (201)
     *       Length: 7 (32 bytes)
     *       Sender SSRC: 0xd2bd4e3e (3535621694)
     *       Source 1
     *           Identifier: 0x00000000 (0)
     *           SSRC contents
     *               Fraction lost: 0x10 / 256
     *               Cumulative number of packets lost: 0x000020
     *           Extended highest sequence number received: 0
     *               Sequence number cycles count: 0
     *               Highest sequence number received: 0
     *           Interarrival jitter: 0
     *           Last SR timestamp: 2262099689 (0x86d4e6e9)
     *           Delay since last SR timestamp: 1 (0 milliseconds)
     */
    uint8_t bufSrSdesPacket[] = {0x81, 0xc8, 0x00, 0x0c, 0xd2, 0xbd, 0x4e, 0x3e, 0xc5, 0x92, 0x86,
            0xd4, 0xe6, 0xe9, 0x78, 0xd5, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00, 0x00, 0x02, 0x00,
            0x00, 0x01, 0x40, 0xd2, 0xbd, 0x4e, 0x3e, 0x10, 0x00, 0x00, 0x20, 0x00, 0x02, 0x00,
            0x02, 0x00, 0x00, 0x00, 0x00, 0x86, 0xd4, 0xe6, 0xe9, 0x00, 0x00, 0x00, 0x01, 0x81,
            0xc9, 0x00, 0x07, 0xd2, 0xbd, 0x4e, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86, 0xd4, 0xe6, 0xe9, 0x00,
            0x00, 0x00, 0x01};

    RtcpConfigInfo rtcpConfigInfo;
    RtpBuffer rtpBuffer(84, bufSrSdesPacket);
    eRTP_STATUS_CODE res = rtcpPacket.decodeRtcpPacket(&rtpBuffer, 0, &rtcpConfigInfo);
    EXPECT_EQ(res, RTP_SUCCESS);

    // Check SR packet.
    std::list<RtcpSrPacket*> SrList = rtcpPacket.getSrPacketList();
    EXPECT_TRUE(SrList.size() != 0);
    RtcpSrPacket* rtcpSrPacket = SrList.front();
    EXPECT_TRUE(rtcpSrPacket != NULL);
    if (rtcpSrPacket == NULL)
        return;

    RtcpHeader* pRtcpHeader = rtcpSrPacket->getRtcpHdrInfo();
    EXPECT_TRUE(pRtcpHeader != NULL);
    if (pRtcpHeader == NULL)
        return;

    EXPECT_EQ(pRtcpHeader->getVersion(), RTP_VERSION_NUM);
    EXPECT_EQ(pRtcpHeader->getPadding(), eRTP_FALSE);
    EXPECT_EQ(pRtcpHeader->getReceptionReportCount(), 1);
    EXPECT_EQ(pRtcpHeader->getPacketType(), RTCP_SR);
    EXPECT_EQ(pRtcpHeader->getLength(), 12 * RTP_WORD_SIZE);
    EXPECT_EQ(pRtcpHeader->getSsrc(), 0xd2bd4e3e);

    tRTP_NTP_TIME* ntpTime = rtcpSrPacket->getNtpTime();
    EXPECT_TRUE(ntpTime != NULL);
    if (ntpTime == NULL)
        return;

    EXPECT_EQ(ntpTime->m_uiNtpHigh32Bits, 3314714324);
    EXPECT_EQ(ntpTime->m_uiNtpLow32Bits, 3874060501);
    EXPECT_EQ(rtcpSrPacket->getRtpTimestamp(), 320);
    EXPECT_EQ(rtcpSrPacket->getSendPktCount(), 2);
    EXPECT_EQ(rtcpSrPacket->getSendOctetCount(), 320);

    RtcpRrPacket* pRRInfo = rtcpSrPacket->getRrPktInfo();
    EXPECT_TRUE(pRRInfo != NULL);
    if (pRRInfo == NULL)
        return;

    std::list<RtcpReportBlock*> reports = pRRInfo->getReportBlockList();
    EXPECT_TRUE(reports.size() != 0);
    if (reports.size() == 0)
        return;

    RtcpReportBlock* report = reports.front();
    EXPECT_TRUE(report != NULL);
    if (report == NULL)
        return;

    EXPECT_EQ(report->getSsrc(), 0xd2bd4e3e);
    EXPECT_EQ((int)report->getFracLost(), 0x10);
    EXPECT_EQ((int)report->getCumNumPktLost(), 0x000020);
    EXPECT_EQ(report->getExtHighSeqRcv(), 131074);
    EXPECT_EQ(report->getJitter(), 0);
    EXPECT_EQ(report->getLastSR(), 2262099689);
    EXPECT_EQ(report->getDelayLastSR(), 1);

    // Check RR packet.
    std::list<RtcpRrPacket*> RrList = rtcpPacket.getRrPacketList();
    EXPECT_TRUE(RrList.size() != 0);
    if (RrList.size() == 0)
        return;

    RtcpRrPacket* pRrPkt = RrList.front();
    EXPECT_TRUE(pRrPkt != NULL);
    if (pRrPkt == NULL)
        return;

    pRtcpHeader = pRrPkt->getRtcpHdrInfo();
    EXPECT_TRUE(pRtcpHeader != NULL);
    if (pRtcpHeader == NULL)
        return;

    EXPECT_EQ(pRtcpHeader->getVersion(), RTP_VERSION_NUM);
    EXPECT_EQ(pRtcpHeader->getPadding(), eRTP_FALSE);
    EXPECT_EQ(pRtcpHeader->getReceptionReportCount(), 1);
    EXPECT_EQ(pRtcpHeader->getPacketType(), RTCP_RR);
    EXPECT_EQ(pRtcpHeader->getLength(), 7 * RTP_WORD_SIZE);
    EXPECT_EQ(pRtcpHeader->getSsrc(), 0xd2bd4e3e);

    reports = pRrPkt->getReportBlockList();
    EXPECT_TRUE(reports.size() != 0);
    if (reports.size() == 0)
        return;

    report = reports.front();
    EXPECT_TRUE(report != NULL);
    if (report == NULL)
        return;

    EXPECT_EQ(report->getSsrc(), 0);
    EXPECT_EQ((int)report->getFracLost(), 0);
    EXPECT_EQ((int)report->getCumNumPktLost(), 0);
    EXPECT_EQ(report->getExtHighSeqRcv(), 0);
    EXPECT_EQ(report->getJitter(), 0);
    EXPECT_EQ(report->getLastSR(), 2262099689);
    EXPECT_EQ(report->getDelayLastSR(), 1);
}

/**
 * Test RTCP packet with Sender Report, Receiver Report and SDES.
 */
TEST_F(RtcpPacketTest, DecodeCompoundSrRrSdesPacket)
{
    RtcpPacket rtcpPacket;

    /*
     * Real-time Transport Control Protocol (Sender Report)
     *   [Stream setup by SDP (frame 1)]
     *   10.. .... = Version: RFC 1889 Version (2)
     *   ..0. .... = Padding: False
     *   ...0 0001 = Reception report count: 1
     *   Packet type: Sender Report (200)
     *   Length: 12 (52 bytes)
     *   Sender SSRC: 0xd2bd4e3e (3535621694)
     *   Timestamp, MSW: 3314714324 (0xc59286d4)
     *   Timestamp, LSW: 4131758539 (0xf645a1cb)
     *   [MSW and LSW as NTP timestamp: Jan 14, 2005 17:58:44.962000000 UTC]
     *   RTP timestamp: 640
     *   Sender's packet count: 4
     *   Sender's octet count: 640
     *   Source 1
     *       Identifier: 0xd2bd4e3e (3535621694)
     *       SSRC contents
     *           Fraction lost: 0 / 256
     *           Cumulative number of packets lost: 0
     *       Extended highest sequence number received: 262148
     *       Interarrival jitter: 0
     *       Last SR timestamp: 2262103621 (0x86d4f645)
     *       Delay since last SR timestamp: 1 (0 milliseconds)
     * Real-time Transport Control Protocol (Receiver Report)
     *   [Stream setup by SDP (frame 1)]
     *   10.. .... = Version: RFC 1889 Version (2)
     *   ..0. .... = Padding: False
     *   ...0 0001 = Reception report count: 1
     *   Packet type: Receiver Report (201)
     *   Length: 7 (32 bytes)
     *   Sender SSRC: 0xd2bd4e3e (3535621694)
     *   Source 1
     *      Identifier: 0x58f33dea (1492336106)
     *       SSRC contents
     *          Fraction lost: 0 / 256
     *           Cumulative number of packets lost: 0
     *       Extended highest sequence number received: 11332
     *       Interarrival jitter: 0
     *       Last SR timestamp: 2262103621 (0x86d4f645)
     *       Delay since last SR timestamp: 1 (0 milliseconds)
     * Real-time Transport Control Protocol (Source description)
     *  [Stream setup by SDP (frame 1)]
     *   10.. .... = Version: RFC 1889 Version (2)
     *   ..0. .... = Padding: False
     *   ...0 0001 = Source count: 1
     *   Packet type: Source description (202)
     *   Length: 7 (32 bytes)
     *   Chunk 1, SSRC/CSRC 0xD2BD4E3E
     *       Identifier: 0xd2bd4e3e (3535621694)
     *       SDES items
     *           Type: CNAME (user and domain) (1)
     *           Length: 20
     *           Text: unknown@200.57.7.204
     *           Type: END (0)
     */
    uint8_t bufSrSdesPacket[] = {0x81, 0xc8, 0x00, 0x0c, 0xd2, 0xbd, 0x4e, 0x3e, 0xc5, 0x92, 0x86,
            0xd4, 0xf6, 0x45, 0xa1, 0xcb, 0x00, 0x00, 0x02, 0x80, 0x00, 0x00, 0x00, 0x04, 0x00,
            0x00, 0x02, 0x80, 0xd2, 0xbd, 0x4e, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
            0x04, 0x00, 0x00, 0x00, 0x00, 0x86, 0xd4, 0xf6, 0x45, 0x00, 0x00, 0x00, 0x01, 0x81,
            0xc9, 0x00, 0x07, 0xd2, 0xbd, 0x4e, 0x3e, 0x58, 0xf3, 0x3d, 0xea, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x2c, 0x44, 0x00, 0x00, 0x00, 0x00, 0x86, 0xd4, 0xf6, 0x45, 0x00,
            0x00, 0x00, 0x01, 0x81, 0xca, 0x00, 0x07, 0xd2, 0xbd, 0x4e, 0x3e, 0x01, 0x14, 0x75,
            0x6e, 0x6b, 0x6e, 0x6f, 0x77, 0x6e, 0x40, 0x32, 0x30, 0x30, 0x2e, 0x35, 0x37, 0x2e,
            0x37, 0x2e, 0x32, 0x30, 0x34, 0x00, 0x00, 0x00, 0x00};

    RtpBuffer rtpBuffer(118, bufSrSdesPacket);

    RtpDt_UChar IPAddress[] = "2600:100e:1008:af4f::1ebe:6851";
    RtcpConfigInfo rtcpConfigInfo;
    addSdesItem(&rtcpConfigInfo, IPAddress, strlen((char*)IPAddress));

    eRTP_STATUS_CODE res = rtcpPacket.decodeRtcpPacket(&rtpBuffer, 0, &rtcpConfigInfo);
    EXPECT_EQ(res, RTP_SUCCESS);

    // Check SR packet.
    std::list<RtcpSrPacket*> SrList = rtcpPacket.getSrPacketList();
    EXPECT_TRUE(SrList.size() != 0);
    RtcpSrPacket* rtcpSrPacket = SrList.front();
    EXPECT_TRUE(rtcpSrPacket != NULL);
    if (rtcpSrPacket == NULL)
        return;

    RtcpHeader* pRtcpHeader = rtcpSrPacket->getRtcpHdrInfo();
    EXPECT_TRUE(pRtcpHeader != NULL);
    if (pRtcpHeader == NULL)
        return;

    EXPECT_EQ(pRtcpHeader->getVersion(), RTP_VERSION_NUM);
    EXPECT_EQ(pRtcpHeader->getPadding(), eRTP_FALSE);
    EXPECT_EQ(pRtcpHeader->getReceptionReportCount(), 1);
    EXPECT_EQ(pRtcpHeader->getPacketType(), RTCP_SR);
    EXPECT_EQ(pRtcpHeader->getLength(), 12 * RTP_WORD_SIZE);
    EXPECT_EQ(pRtcpHeader->getSsrc(), 0xd2bd4e3e);

    tRTP_NTP_TIME* ntpTime = rtcpSrPacket->getNtpTime();
    EXPECT_TRUE(ntpTime != NULL);
    if (ntpTime == NULL)
        return;

    EXPECT_EQ(ntpTime->m_uiNtpHigh32Bits, 3314714324);
    EXPECT_EQ(ntpTime->m_uiNtpLow32Bits, 4131758539);
    EXPECT_EQ(rtcpSrPacket->getRtpTimestamp(), 640);
    EXPECT_EQ(rtcpSrPacket->getSendPktCount(), 4);
    EXPECT_EQ(rtcpSrPacket->getSendOctetCount(), 640);

    RtcpRrPacket* pRRInfo = rtcpSrPacket->getRrPktInfo();
    EXPECT_TRUE(pRRInfo != NULL);
    if (pRRInfo == NULL)
        return;

    std::list<RtcpReportBlock*> reports = pRRInfo->getReportBlockList();
    EXPECT_TRUE(reports.size() != 0);
    if (reports.size() == 0)
        return;

    RtcpReportBlock* report = reports.front();
    EXPECT_TRUE(report != NULL);
    if (report == NULL)
        return;

    EXPECT_EQ(report->getSsrc(), 0xd2bd4e3e);
    EXPECT_EQ((int)report->getFracLost(), 0);
    EXPECT_EQ((int)report->getCumNumPktLost(), 0);
    EXPECT_EQ(report->getExtHighSeqRcv(), 262148);
    EXPECT_EQ(report->getJitter(), 0);
    EXPECT_EQ(report->getLastSR(), 2262103621);
    EXPECT_EQ(report->getDelayLastSR(), 1);

    // Check RR packet.
    std::list<RtcpRrPacket*> RrList = rtcpPacket.getRrPacketList();
    EXPECT_TRUE(RrList.size() != 0);
    if (RrList.size() == 0)
        return;

    RtcpRrPacket* pRrPkt = RrList.front();
    EXPECT_TRUE(pRrPkt != NULL);
    if (pRrPkt == NULL)
        return;

    pRtcpHeader = pRrPkt->getRtcpHdrInfo();
    EXPECT_TRUE(pRtcpHeader != NULL);
    if (pRtcpHeader == NULL)
        return;

    EXPECT_EQ(pRtcpHeader->getVersion(), RTP_VERSION_NUM);
    EXPECT_EQ(pRtcpHeader->getPadding(), eRTP_FALSE);
    EXPECT_EQ(pRtcpHeader->getReceptionReportCount(), 1);
    EXPECT_EQ(pRtcpHeader->getPacketType(), RTCP_RR);
    EXPECT_EQ(pRtcpHeader->getLength(), 7 * RTP_WORD_SIZE);
    EXPECT_EQ(pRtcpHeader->getSsrc(), 0xd2bd4e3e);

    reports = pRrPkt->getReportBlockList();
    EXPECT_TRUE(reports.size() != 0);
    if (reports.size() == 0)
        return;

    report = reports.front();
    EXPECT_TRUE(report != NULL);
    if (report == NULL)
        return;

    EXPECT_EQ(report->getSsrc(), 0x58f33dea);
    EXPECT_EQ((int)report->getFracLost(), 0);
    EXPECT_EQ((int)report->getCumNumPktLost(), 0);
    EXPECT_EQ(report->getExtHighSeqRcv(), 11332);
    EXPECT_EQ(report->getJitter(), 0);
    EXPECT_EQ(report->getLastSR(), 2262103621);
    EXPECT_EQ(report->getDelayLastSR(), 1);

    // Check SDES
    RtcpSdesPacket* pRtcpSdesPacket = rtcpPacket.getSdesPacket();
    EXPECT_TRUE(pRtcpSdesPacket != NULL);
    if (pRtcpSdesPacket == NULL)
        return;

    std::list<RtcpChunk*> pSdesChunks = pRtcpSdesPacket->getSdesChunkList();
    EXPECT_EQ(pSdesChunks.size(), 1);
    RtcpChunk* chunk = pSdesChunks.front();
    EXPECT_TRUE(chunk != NULL);
    if (chunk == NULL)
        return;

    std::list<tRTCP_SDES_ITEM*> sdesItemList = chunk->getSdesItemList();
    EXPECT_EQ(sdesItemList.size(), 1);
    tRTCP_SDES_ITEM* sdesItem = sdesItemList.front();
    EXPECT_TRUE(sdesItem != NULL);
    if (sdesItem == NULL)
        return;

    EXPECT_EQ(sdesItem->ucType, 1);
    EXPECT_EQ(sdesItem->ucLength, 20);
    EXPECT_EQ(strncmp((char*)sdesItem->pValue, (char*)"unknown@200.57.7.204", 20), 0);

    pRtcpHeader = pRtcpSdesPacket->getRtcpHdrInfo();
    EXPECT_TRUE(pRtcpHeader != NULL);
    if (pRtcpHeader == NULL)
        return;

    EXPECT_EQ(pRtcpHeader->getVersion(), RTP_VERSION_NUM);
    EXPECT_EQ(pRtcpHeader->getPadding(), eRTP_FALSE);
    EXPECT_EQ(pRtcpHeader->getReceptionReportCount(), 1);
    EXPECT_EQ(pRtcpHeader->getPacketType(), RTCP_SDES);
    EXPECT_EQ(pRtcpHeader->getLength(), 7 * RTP_WORD_SIZE);
    EXPECT_EQ(pRtcpHeader->getSsrc(), 0xd2bd4e3e);
}

/**
 * Test RTCP BYE packet.
 */
TEST_F(RtcpPacketTest, TestDecodeByePacket)
{
    RtcpPacket rtcpPacket;

    /*
     * Real-time Transport Control Protocol (Sender Report)
     * 10.. .... = Version: RFC 1889 Version (2)
     * ..1. .... = Padding: True
     * ...0 0011 = Reception report count: 3
     * Packet type: Bye (203)
     * Length: 6 (28 bytes)
     * SSRC 1: 0xb1c8cb02 (2982726402)
     * SSRC 1: 0xb1c8cb03 (2982726403)
     * SSRC 1: 0xb1c8cb04 (2982726404)
     * Length: 8
     * Reason for leaving: teardown
     * padding: 0x000003
     */
    uint8_t bufPacket[] = {0xA3, 0xcb, 0x00, 0x6, 0xb1, 0xc8, 0xcb, 0x02, 0xb1, 0xc8, 0xcb, 0x03,
            0xb1, 0xc8, 0xcb, 0x04, 0x08, 0x74, 0x65, 0x61, 0x72, 0x64, 0x6F, 0x77, 0x6E, 0x00,
            0x00, 0x03};

    RtcpConfigInfo rtcpConfigInfo;
    RtpBuffer rtpBuffer(28, bufPacket);
    eRTP_STATUS_CODE res = rtcpPacket.decodeRtcpPacket(&rtpBuffer, 0, &rtcpConfigInfo);
    EXPECT_EQ(res, RTP_SUCCESS);

    RtcpByePacket* pByePacket = rtcpPacket.getByePacket();
    EXPECT_TRUE(pByePacket != 0);
    if (pByePacket == NULL)
        return;

    RtcpHeader* pRtcpHeader = pByePacket->getRtcpHdrInfo();
    EXPECT_TRUE(pRtcpHeader != NULL);
    if (pRtcpHeader == NULL)
        return;

    EXPECT_EQ(pRtcpHeader->getVersion(), RTP_VERSION_NUM);
    EXPECT_EQ(pRtcpHeader->getPadding(), eRTP_TRUE);
    EXPECT_EQ(pRtcpHeader->getReceptionReportCount(), 3);
    EXPECT_EQ(pRtcpHeader->getPacketType(), RTCP_BYE);
    EXPECT_EQ(pRtcpHeader->getLength(), 6 * RTP_WORD_SIZE);
    EXPECT_EQ(pRtcpHeader->getSsrc(), 0xb1c8cb02);

    std::list<RtpDt_UInt32*> ssrcList = pByePacket->getSsrcList();
    EXPECT_TRUE(ssrcList.size() == 2);
    EXPECT_EQ(*ssrcList.front(), (RtpDt_UInt32)0xb1c8cb03);
    ssrcList.pop_front();
    EXPECT_EQ(*ssrcList.front(), (RtpDt_UInt32)0xb1c8cb04);

    RtpBuffer* reason = pByePacket->getReason();
    EXPECT_TRUE(reason != NULL);
    if (reason == NULL)
        return;

    EXPECT_EQ(reason->getLength(), 8);
    EXPECT_EQ(strcmp((char*)reason->getBuffer(), (char*)"teardown"), 0);
}

/**
 * Test RTCP APP packet.
 */
TEST_F(RtcpPacketTest, TestDecodeAppPacket)
{
    RtcpPacket rtcpPacket;

    /*
     * Real-time Transport Control Protocol (Sender Report)
     * 10.. .... = Version: RFC 1889 Version (2)
     * ..0. .... = Padding: False
     * ...0 1111 = SubType
     * Packet type: App (204)
     * Length: 10 (40 bytes)
     * SSRC : 0xb1c8cb02 (2982726402)
     * App defined packet name: TEST
     * Application data: This is a test application data.
     */
    uint8_t bufPacket[] = {0x8F, 0xcc, 0x00, 0x0a, 0xb1, 0xc8, 0xcb, 0x02, 0x54, 0x45, 0x53, 0x54,
            0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x61, 0x20, 0x74, 0x65, 0x73, 0x74,
            0x20, 0x61, 0x70, 0x70, 0x6c, 0x69, 0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x64,
            0x61, 0x74, 0x61, 0x2e};

    RtcpConfigInfo rtcpConfigInfo;
    RtpBuffer rtpBuffer(44, bufPacket);
    eRTP_STATUS_CODE res = rtcpPacket.decodeRtcpPacket(&rtpBuffer, 0, &rtcpConfigInfo);
    EXPECT_EQ(res, RTP_SUCCESS);

    RtcpAppPacket* pAppPacket = rtcpPacket.getAppPacket();
    EXPECT_TRUE(pAppPacket != 0);
    if (pAppPacket == NULL)
        return;

    RtcpHeader* pRtcpHeader = pAppPacket->getRtcpHdrInfo();
    EXPECT_TRUE(pRtcpHeader != NULL);
    if (pRtcpHeader == NULL)
        return;

    EXPECT_EQ(pRtcpHeader->getVersion(), RTP_VERSION_NUM);
    EXPECT_EQ(pRtcpHeader->getPadding(), eRTP_FALSE);
    EXPECT_EQ(pRtcpHeader->getReceptionReportCount(), 0x0f);
    EXPECT_EQ(pRtcpHeader->getPacketType(), RTCP_APP);
    EXPECT_EQ(pRtcpHeader->getLength(), 10 * RTP_WORD_SIZE);
    EXPECT_EQ(pRtcpHeader->getSsrc(), 0xb1c8cb02);
    RtpDt_UInt32 appPktName = pAppPacket->getName();
    EXPECT_EQ(memcmp(&appPktName, (char*)"TEST", 4), 0);
    RtpBuffer* pAppData = pAppPacket->getAppData();
    EXPECT_EQ(memcmp(pAppData->getBuffer(), (char*)"This is a test application data.", 32), 0);
    EXPECT_EQ(pAppData->getLength(), 32);
}

/**
 * Test RTCP Feedback packet decoding.
 */
TEST_F(RtcpPacketTest, TestDecodeFBPacket)
{
    RtcpPacket rtcpPacket;

    /*
     * Real-time Transport Control Protocol (Payload- specific)
     * 10.. .... = Version: RFC 1889 Version (2)
     * ..0. .... = Padding: False
     * ...0 0001 = PLI: Picture Loss Indication.
     * Packet type: PSFB (206)
     * Length: 2 (8 bytes)
     * Sender SSRC : 0xb1c8cb02 (2982726402)
     * Media SSRC : 0xb1c8cb03 (2982726402)
     *
     * Real-time Transport Control Protocol (Generic RTP Feedback): TMMBR: 2097152
     * 10.. .... = Version: RFC 1889 Version (2)
     * ..0. .... = Padding: False
     * ...0 0011 = PLI: TMMBR: Temp Max Media stream Bit Rate Request.
     * Packet type: PSFB (205)
     * Length: 4 (16 bytes)
     * Sender SSRC : 0xb1c8cb02 (2982726402)
     * Media SSRC : 0xb1c8cb03 (2982726402)
     * 8bytes of test data: TMMBR***
     */
    uint8_t bufPacket[] = {0x81, 0xce, 0x00, 0x02, 0xb1, 0xc8, 0xcb, 0x02, 0xb1, 0xc8, 0xcb, 0x03,
            0x83, 0xcd, 0x00, 0x04, 0xb1, 0xc8, 0xcb, 0x02, 0xb1, 0xc8, 0xcb, 0x03, 0x54, 0x4d,
            0x4d, 0x42, 0x52, 0x2a, 0x2a, 0x2a};

    RtcpConfigInfo rtcpConfigInfo;
    RtpBuffer rtpBuffer(32, bufPacket);
    eRTP_STATUS_CODE res = rtcpPacket.decodeRtcpPacket(&rtpBuffer, 0, &rtcpConfigInfo);
    EXPECT_EQ(res, RTP_SUCCESS);

    std::list<RtcpFbPacket*> fbpktList = rtcpPacket.getFbPacketList();
    EXPECT_EQ(fbpktList.size(), 2);
    RtcpFbPacket* fbpkt = fbpktList.front();
    EXPECT_TRUE(fbpkt != NULL);
    if (fbpkt == NULL)
        return;

    RtcpHeader* pRtcpHeader = fbpkt->getRtcpHdrInfo();
    EXPECT_TRUE(pRtcpHeader != NULL);
    if (pRtcpHeader == NULL)
        return;

    EXPECT_EQ(pRtcpHeader->getVersion(), RTP_VERSION_NUM);
    EXPECT_EQ(pRtcpHeader->getPadding(), eRTP_FALSE);
    EXPECT_EQ(pRtcpHeader->getReceptionReportCount(), 1);
    EXPECT_EQ(pRtcpHeader->getPacketType(), RTCP_PSFB);
    EXPECT_EQ(pRtcpHeader->getLength(), 2 * RTP_WORD_SIZE);
    EXPECT_EQ(pRtcpHeader->getSsrc(), 0xb1c8cb02);
    EXPECT_EQ(fbpkt->getMediaSsrc(), 0xb1c8cb03);

    fbpktList.pop_front();
    fbpkt = fbpktList.front();
    EXPECT_TRUE(fbpkt != NULL);
    if (fbpkt == NULL)
        return;

    pRtcpHeader = fbpkt->getRtcpHdrInfo();
    EXPECT_TRUE(pRtcpHeader != NULL);
    if (pRtcpHeader == NULL)
        return;

    EXPECT_EQ(pRtcpHeader->getVersion(), RTP_VERSION_NUM);
    EXPECT_EQ(pRtcpHeader->getPadding(), eRTP_FALSE);
    EXPECT_EQ(pRtcpHeader->getReceptionReportCount(), 3);
    EXPECT_EQ(pRtcpHeader->getPacketType(), RTCP_RTPFB);
    EXPECT_EQ(pRtcpHeader->getLength(), 4 * RTP_WORD_SIZE);
    EXPECT_EQ(pRtcpHeader->getSsrc(), 0xb1c8cb02);
    EXPECT_EQ(fbpkt->getMediaSsrc(), 0xb1c8cb03);
}

/**
 * Test RTCP packet with only SR header.
 */
TEST_F(RtcpPacketTest, DecodeOnlyRtcpSRHeader)
{
    RtcpPacket rtcpPacket;

    /*
     * Real-time Transport Control Protocol (Sender Report)
     * 10.. .... = Version: RFC 1889 Version (2)
     * ..0. .... = Padding: False
     * ...0 0000 = Reception report count: 0
     * Packet type: Sender Report (200)
     * Length: 0 (0 bytes)
     */
    uint8_t bufPacket[] = {0x80, 0xc8, 0x00, 0x0};

    RtcpConfigInfo rtcpConfigInfo;
    RtpBuffer rtpBuffer(4, bufPacket);
    eRTP_STATUS_CODE res = rtcpPacket.decodeRtcpPacket(&rtpBuffer, 0, &rtcpConfigInfo);
    EXPECT_EQ(res, RTP_SUCCESS);

    RtcpHeader pRtcpHeader = rtcpPacket.getHeader();
    EXPECT_EQ(pRtcpHeader.getVersion(), RTP_VERSION_NUM);
    EXPECT_EQ(pRtcpHeader.getPadding(), eRTP_FALSE);
    EXPECT_EQ(pRtcpHeader.getReceptionReportCount(), 0);
    EXPECT_EQ(pRtcpHeader.getPacketType(), RTCP_SR);
    EXPECT_EQ(pRtcpHeader.getLength(), 0 * RTP_WORD_SIZE);
}

TEST_F(RtcpPacketTest, CheckAllGetSets)
{
    RtcpPacket rtcpPacket;

    RtcpSdesPacket* sdesPacket = new RtcpSdesPacket();
    rtcpPacket.setSdesPacketData(sdesPacket);
    EXPECT_EQ(sdesPacket, rtcpPacket.getSdesPacket());

    RtcpByePacket* byePacket = new RtcpByePacket();
    rtcpPacket.setByePacketData(byePacket);
    EXPECT_EQ(byePacket, rtcpPacket.getByePacket());

    RtcpAppPacket* appPacket = new RtcpAppPacket();
    rtcpPacket.setAppPktData(appPacket);
    EXPECT_EQ(appPacket, rtcpPacket.getAppPacket());

    RtcpXrPacket* XrPacket = new RtcpXrPacket();
    rtcpPacket.setXrPacket(XrPacket);
    EXPECT_EQ(XrPacket, rtcpPacket.getXrPacket());

    RtcpSrPacket* srPacket1 = new RtcpSrPacket();
    RtcpSrPacket* srPacket2 = new RtcpSrPacket();
    rtcpPacket.addSrPacketData(srPacket1);
    rtcpPacket.addSrPacketData(srPacket2);
    std::list<RtcpSrPacket*> srList = rtcpPacket.getSrPacketList();
    EXPECT_TRUE(srPacket1 == srList.front());
    srList.pop_front();
    EXPECT_TRUE(srPacket2 == srList.front());
    srList.clear();

    RtcpRrPacket* RrPacket1 = new RtcpRrPacket();
    RtcpRrPacket* RrPacket2 = new RtcpRrPacket();
    rtcpPacket.addRrPacketData(RrPacket1);
    rtcpPacket.addRrPacketData(RrPacket2);
    std::list<RtcpRrPacket*> rrList = rtcpPacket.getRrPacketList();
    EXPECT_TRUE(RrPacket1 == rrList.front());
    rrList.pop_front();
    EXPECT_TRUE(RrPacket2 == rrList.front());
    rrList.clear();

    RtcpFbPacket* FbPacket1 = new RtcpFbPacket();
    RtcpFbPacket* FbPacket2 = new RtcpFbPacket();
    rtcpPacket.addFbPacketData(FbPacket1);
    rtcpPacket.addFbPacketData(FbPacket2);
    std::list<RtcpFbPacket*> fbList = rtcpPacket.getFbPacketList();
    EXPECT_TRUE(FbPacket1 == fbList.front());
    fbList.pop_front();
    EXPECT_TRUE(FbPacket2 == fbList.front());
    fbList.clear();
}