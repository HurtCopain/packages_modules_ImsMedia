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

#include <RtpDecoderNode.h>
#include <ImsMediaTrace.h>
#include <AudioConfig.h>

RtpDecoderNode::RtpDecoderNode() {
    mRtpSession = NULL;
    mReceivingSSRC = 0;
    mInactivityTime = 0;
    mNoRtpTime = 0;
}

RtpDecoderNode::~RtpDecoderNode() {
    if (mRtpSession) {
        mRtpSession->SetRtpDecoderListener(NULL);
        mRtpSession->StopRtp();
        IRtpSession::ReleaseInstance(mRtpSession);
    }
}

BaseNode* RtpDecoderNode::GetInstance() {
    BaseNode* pNode = new RtpDecoderNode();
    if (pNode == NULL) {
        IMLOGE0("[GetInstance] Can't create RtpDecoderNode");
    }

    return pNode;
}

void RtpDecoderNode::ReleaseInstance(BaseNode* pNode) {
    delete (RtpDecoderNode*)pNode;
}

BaseNodeID RtpDecoderNode::GetNodeID() {
    return NODEID_RTPDECODER;
}

ImsMediaResult RtpDecoderNode::Start() {
    if (mRtpSession == NULL) {
        mRtpSession = IRtpSession::GetInstance(mMediaType, mLocalAddress, mPeerAddress);
        if (mRtpSession == NULL) {
            IMLOGE0("[Start] - Can't create rtp session");
            return IMS_MEDIA_ERROR_UNKNOWN;
        }
    }

    mRtpSession->SetRtpDecoderListener(this);
    mRtpSession->StartRtp();
    mReceivingSSRC = 0;
    mInactivityTime = 0;
    mNoRtpTime = 0;
    mNodeState = NODESTATE_RUNNING;
    return IMS_MEDIA_OK;
}

void RtpDecoderNode::Stop() {
    mReceivingSSRC = 0;
    mNodeState = NODESTATE_STOPPED;
}

void RtpDecoderNode::OnDataFromFrontNode(ImsMediaSubType subtype, uint8_t* pData,
    uint32_t nDataSize, uint32_t nTimestamp, bool bMark, uint32_t nSeqNum,
    ImsMediaSubType nDataType) {
    IMLOGD_PACKET6(IM_PACKET_LOG_RTP,
        "[OnDataFromFrontNode] subtype[%d] Size[%d], TS[%d], Mark[%d], Seq[%d], datatype[%d]",
        subtype, nDataSize, nTimestamp, bMark, nSeqNum, nDataType);
    mRtpSession->ProcRtpPacket(pData, nDataSize);
}

bool RtpDecoderNode::IsRunTime() {
    return true;
}

bool RtpDecoderNode::IsSourceNode() {
    return false;
}

void RtpDecoderNode::SetConfig(void* config) {
    IMLOGD0("[SetConfig]");
    if (config == NULL) return;
    RtpConfig *pConfig = reinterpret_cast<RtpConfig*>(config);
    mPeerAddress = RtpAddress(pConfig->getRemoteAddress().c_str(), pConfig->getRemotePort());
    SetSamplingRate(pConfig->getSamplingRateKHz() * 1000);
    IMLOGD2("[SetConfig] peer Ip[%s], port[%d]", mPeerAddress.ipAddress,
        mPeerAddress.port);
}

bool RtpDecoderNode::IsSameConfig(void* config) {
    if (config == NULL) return true;
    RtpConfig* pConfig = reinterpret_cast<RtpConfig*>(config);
    RtpAddress peerAddress = RtpAddress(pConfig->getRemoteAddress().c_str(),
        pConfig->getRemotePort());

    return (mPeerAddress == peerAddress
        && mSamplingRate == (pConfig->getSamplingRateKHz() * 1000));
}

void RtpDecoderNode::OnMediaDataInd(unsigned char* pData, uint32_t nDataSize, uint32_t nTimestamp,
    bool bMark, uint16_t nSeqNum, uint32_t nPayloadType, uint32_t nSSRC,
    bool bExtension, uint16_t nExtensionData) {
    (void)nPayloadType;
    (void)bExtension;
    (void)nExtensionData;

    IMLOGD_PACKET6(IM_PACKET_LOG_RTP,
        "[OnMediaDataInd] type[%d] Size[%d], TS[%d], Mark[%d], Seq[%d], SamplingRate[%d]",
        mMediaType, nDataSize, nTimestamp, bMark, nSeqNum, mSamplingRate);

    // no need to change to timestamp to msec in video or text packet
    if (mMediaType != IMS_MEDIA_VIDEO && mSamplingRate > 1000) {
        nTimestamp = nTimestamp / (mSamplingRate / 1000);
        if (mReceivingSSRC != nSSRC) {
            IMLOGD3("[OnMediaDataInd] type[%d] SSRC changed, received SSRC[%x], nSSRC[%x]",
                mMediaType, mReceivingSSRC, nSSRC);
            mReceivingSSRC = nSSRC;
        }
    }

    SendDataToRearNode(MEDIASUBTYPE_RTPPAYLOAD, pData, nDataSize,
        nTimestamp, bMark, nSeqNum);
}

void RtpDecoderNode::OnNumReceivedPacket(uint32_t nNumRtpPacket) {
    IMLOGD_PACKET2(IM_PACKET_LOG_RTCP,
        "[OnNumReceivedPacket] mediaType[%d], numRTP[%d]", mMediaType, nNumRtpPacket);

    if (nNumRtpPacket == 0) {
        mNoRtpTime++;
    } else{
        mNoRtpTime = 0;
    }

    if (mInactivityTime != 0 && mNoRtpTime == mInactivityTime) {
        if (mCallback != NULL) {
            mCallback->SendEvent(EVENT_NOTIFY_MEDIA_INACITIVITY, RTP, mInactivityTime);
        }
    }
}

void RtpDecoderNode::SetLocalAddress(const RtpAddress address) {
    mLocalAddress = address;
}

void RtpDecoderNode::SetPeerAddress(const RtpAddress address) {
    mPeerAddress = address;
}

void RtpDecoderNode::SetSamplingRate(const uint32_t data) {
    mSamplingRate = data;
}

void RtpDecoderNode::SetInactivityTimerSec(const uint32_t time) {
    mInactivityTime = time;
}