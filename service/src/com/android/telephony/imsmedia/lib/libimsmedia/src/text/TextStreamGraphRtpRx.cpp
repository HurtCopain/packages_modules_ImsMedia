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

#include <TextStreamGraphRtpRx.h>
#include <ImsMediaTrace.h>
#include <ImsMediaNetworkUtil.h>
#include <TextConfig.h>
#include <RtpDecoderNode.h>
#include <SocketReaderNode.h>
#include <TextRtpPayloadDecoderNode.h>
#include <TextRendererNode.h>

TextStreamGraphRtpRx::TextStreamGraphRtpRx(BaseSessionCallback* callback, int localFd) :
        TextStreamGraph(callback, localFd)
{
}

TextStreamGraphRtpRx::~TextStreamGraphRtpRx() {}

ImsMediaResult TextStreamGraphRtpRx::create(RtpConfig* config)
{
    IMLOGD1("[createGraph] state[%d]", mGraphState);

    if (config == NULL)
    {
        return RESULT_INVALID_PARAM;
    }

    mConfig = new TextConfig(reinterpret_cast<TextConfig*>(config));

    char localIp[MAX_IP_LEN];
    uint32_t localPort = 0;
    ImsMediaNetworkUtil::getLocalIpPortFromSocket(mLocalFd, localIp, MAX_IP_LEN, localPort);
    RtpAddress localAddress(localIp, localPort);

    BaseNode* pNodeSocketReader = new SocketReaderNode(mCallback);
    pNodeSocketReader->SetMediaType(IMS_MEDIA_TEXT);
    ((SocketReaderNode*)pNodeSocketReader)->SetLocalFd(mLocalFd);
    ((SocketReaderNode*)pNodeSocketReader)->SetLocalAddress(localAddress);
    ((SocketReaderNode*)pNodeSocketReader)->SetProtocolType(RTP);
    pNodeSocketReader->SetConfig(config);
    AddNode(pNodeSocketReader);

    BaseNode* pNodeRtpDecoder = new RtpDecoderNode(mCallback);
    pNodeRtpDecoder->SetMediaType(IMS_MEDIA_TEXT);
    pNodeRtpDecoder->SetConfig(mConfig);
    ((RtpDecoderNode*)pNodeRtpDecoder)->SetLocalAddress(localAddress);
    AddNode(pNodeRtpDecoder);
    pNodeSocketReader->ConnectRearNode(pNodeRtpDecoder);

    BaseNode* pNodeRtpPayloadDecoder = new TextRtpPayloadDecoderNode(mCallback);
    pNodeRtpPayloadDecoder->SetMediaType(IMS_MEDIA_TEXT);
    pNodeRtpPayloadDecoder->SetConfig(mConfig);
    AddNode(pNodeRtpPayloadDecoder);
    pNodeRtpDecoder->ConnectRearNode(pNodeRtpPayloadDecoder);

    BaseNode* pNodeRenderer = new TextRendererNode(mCallback);
    pNodeRenderer->SetMediaType(IMS_MEDIA_TEXT);
    pNodeRenderer->SetConfig(mConfig);
    AddNode(pNodeRenderer);
    pNodeRtpPayloadDecoder->ConnectRearNode(pNodeRenderer);
    setState(StreamState::kStreamStateCreated);
    return RESULT_SUCCESS;
}

ImsMediaResult TextStreamGraphRtpRx::update(RtpConfig* config)
{
    IMLOGD1("[update] state[%d]", mGraphState);

    if (config == NULL)
    {
        return RESULT_INVALID_PARAM;
    }

    TextConfig* pConfig = reinterpret_cast<TextConfig*>(config);

    if (*mConfig == *pConfig)
    {
        IMLOGD0("[update] no update");
        return RESULT_SUCCESS;
    }

    if (mConfig != NULL)
    {
        delete mConfig;
        mConfig = NULL;
    }

    mConfig = new TextConfig(pConfig);

    if (mConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_NO_FLOW ||
            mConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_SEND_ONLY ||
            mConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_INACTIVE)
    {
        IMLOGD0("[update] pause RX");
        return stop();
    }

    ImsMediaResult ret = RESULT_NOT_READY;

    if (mGraphState == kStreamStateRunning)
    {
        mScheduler->Stop();

        for (auto& node : mListNodeStarted)
        {
            if (node != NULL)
            {
                IMLOGD1("[update] update node[%s]", node->GetNodeName());
                ret = node->UpdateConfig(mConfig);
                if (ret != RESULT_SUCCESS)
                {
                    IMLOGE2("[update] error in update node[%s], ret[%d]", node->GetNodeName(), ret);
                }
            }
        }

        mScheduler->Start();
    }
    else if (mGraphState == kStreamStateCreated)
    {
        for (auto& node : mListNodeToStart)
        {
            if (node != NULL)
            {
                IMLOGD1("[update] update node[%s]", node->GetNodeName());
                ret = node->UpdateConfig(mConfig);

                if (ret != RESULT_SUCCESS)
                {
                    IMLOGE2("[update] error in update node[%s], ret[%d]", node->GetNodeName(), ret);
                }
            }
        }
    }

    if (mGraphState == kStreamStateCreated &&
            (pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_RECEIVE_ONLY ||
                    pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_SEND_RECEIVE))
    {
        IMLOGD0("[update] resume RX");
        return start();
    }

    return ret;
}

ImsMediaResult TextStreamGraphRtpRx::start()
{
    IMLOGD1("[start] state[%d]", mGraphState);

    if (mConfig == NULL)
    {
        return RESULT_INVALID_PARAM;
    }

    TextConfig* pConfig = reinterpret_cast<TextConfig*>(mConfig);

    if (pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_NO_FLOW ||
            pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_SEND_ONLY ||
            mConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_INACTIVE)
    {
        IMLOGD1("[start] direction[%d] no need to start", pConfig->getMediaDirection());
        return RESULT_SUCCESS;
    }

    ImsMediaResult result = startNodes();

    if (result != RESULT_SUCCESS)
    {
        setState(StreamState::kStreamStateCreated);
        mCallback->SendEvent(kImsMediaEventNotifyError, result, kStreamModeRtpRx);
        return result;
    }

    setState(StreamState::kStreamStateRunning);
    return RESULT_SUCCESS;
}

bool TextStreamGraphRtpRx::setMediaQualityThreshold(MediaQualityThreshold* threshold)
{
    if (threshold != NULL)
    {
        BaseNode* node = findNode(kNodeIdRtpDecoder);

        if (node != NULL)
        {
            RtpDecoderNode* decoder = reinterpret_cast<RtpDecoderNode*>(node);
            decoder->SetInactivityTimerSec(threshold->getRtpInactivityTimerMillis() / 1000);
            return true;
        }
    }

    return false;
}