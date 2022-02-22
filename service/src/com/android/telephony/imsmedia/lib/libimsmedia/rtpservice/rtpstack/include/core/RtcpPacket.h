/**
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

/** \addtogroup  RTP_Stack
 *  @{
 */

#ifndef __RTCP_PACKET_H__
#define __RTCP_PACKET_H__

#include <RtpGlobal.h>
#include <RtcpSrPacket.h>
#include <RtcpRrPacket.h>
#include <RtcpAppPacket.h>
#include <RtcpSdesPacket.h>
#include <RtcpByePacket.h>
#include <RtcpConfigInfo.h>
#include <RtcpXrPacket.h>
#include <RtcpFbPacket.h>

 /**
 * @class   RtcpPacket.h
 * @brief   It holds RTCP information
 */
class RtcpPacket
{
    private:
        // list of RtcpSrPacket
        std::list<RtcpSrPacket *>  m_objSrPktList;

        // list of RtcpRrPacket
        std::list<RtcpRrPacket *>  m_objRrPktList;

        // list of RtcpFbPacket
        std::list<RtcpFbPacket *>  m_objFbPktList;

        // SDES packet information
        RtcpSdesPacket *m_pobjSdesPkt;

        // Bye packet information
        RtcpByePacket *m_pobjByePkt;

        // App packet information
        RtcpAppPacket *m_pobjAppPkt;
        RtcpFbPacket *m_pobjRtcpFbPkt;

        // XR packet information
        RtcpXrPacket *m_pobjRtcpXrPkt;

        /**
         * It adds RtcpFbPacket pointer to m_objFbPktList
         */
        eRTP_STATUS_CODE addFbPacketData(IN RtcpFbPacket *pobjFbPkt);

    public:

        RtcpPacket();
        ~RtcpPacket();

        /**
         * get method for m_objSrPktList
         */
        std::list<RtcpSrPacket *>& getSrPacketList();

        /**
         * get method for m_objRrPktList
         */
        std::list<RtcpRrPacket *>& getRrPacketList();

        /**
         * get method for m_objFbPktList
         */
        std::list<RtcpFbPacket *>& getFbPacketList();

        /**
         * get method for m_pobjSdesPkt
         */
        RtcpSdesPacket* getSdesPacket();

        /**
         * set method for m_pobjSdesPkt
         */
        RtpDt_Void setSdesPacketData(IN RtcpSdesPacket* pobjSdesData);

        /**
         * get method for m_pobjByePkt
         */
        RtcpByePacket* getByePacket();

        /**
         * set method for m_pobjByePkt
         */
        RtpDt_Void setByePacketData(IN RtcpByePacket* pobjByePktData);

        /**
         * get method for m_pobjAppPkt
         */
        RtcpAppPacket* getAppPacket();

        /**
         * set method for m_pobjAppPkt
         */
        RtpDt_Void setAppPktData(IN RtcpAppPacket* pobjAppData);

        /**
         * get method for m_pobjRtcpFbPkt
         */
        RtcpFbPacket* getRtcpFbPacket();

        /**
         * set method for m_pobjRtcpFbPkt
         */
        RtpDt_Void setRtcpFbPktData(IN RtcpFbPacket* pobjRtcpFbData);

        /**
         * It adds RtcpRrPacket pointer to m_objRrPktList
         */
        eRTP_STATUS_CODE addRrPacketData(IN RtcpRrPacket *pobjRrPkt);

        /**
         * It adds RtcpSrPacket pointer to m_objSrPktList
         */
        eRTP_STATUS_CODE addSrPacketData(IN RtcpSrPacket *pobjSrPkt);

        /**
         * get method for m_pobjRtcpXrPkt
         */
        RtcpXrPacket* getXrPacket();

        /**
         * set method for m_pobjRtcpXrPkt
         */
        RtpDt_Void setXrPktData(IN RtcpXrPacket* pobjRtcpXrData);

        /**
         * Decodes and stores the information of the RTCP packet
         * This function does not allocate memory required for decoding.
         *
         * @param   pobjRtcpPktBuf  Memory for the buffer is pre-allocated by caller
         * @param   usExtHdrLen     RTCP extension header length
         *
         * @return  RTP_SUCCESS on successful decoding
         */
        eRTP_STATUS_CODE decodeRtcpPacket(IN RtpBuffer* pobjRtcpPktBuf,
                                     IN RtpDt_UInt16 usExtHdrLen,
                                     IN RtcpConfigInfo *pobjRtcpCfgInfo);

        /**
         * Performs the encoding of the RTCP packet.
         * This function does not allocate memory required for encoding.
         *
         * @param pobjRtcpPktBuf    Memory for the buffer is pre-allocated by caller
         *
         * @return RTP_SUCCESS on successful encoding
         */
        eRTP_STATUS_CODE formRtcpPacket(OUT RtpBuffer* pobjRtcpPktBuf);

};  // end of RtcpPacket

#endif    //__RTCP_PACKET_H__

/** @}*/
