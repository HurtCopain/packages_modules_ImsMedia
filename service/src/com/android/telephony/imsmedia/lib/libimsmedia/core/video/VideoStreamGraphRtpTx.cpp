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

#include <VideoStreamGraphRtpTx.h>
#include <ImsMediaTrace.h>
#include <ImsMediaNetworkUtil.h>
#include <VideoConfig.h>
#include <RtpEncoderNode.h>
#include <SocketWriterNode.h>
#include <VideoRtpPayloadEncoderNode.h>
#include <IVideoSourceNode.h>

VideoStreamGraphRtpTx::VideoStreamGraphRtpTx(BaseSessionCallback* callback, int localFd) :
        VideoStreamGraph(callback, localFd)
{
    mSurface = NULL;
    mVideoMode = -1;
}

VideoStreamGraphRtpTx::~VideoStreamGraphRtpTx() {}

ImsMediaResult VideoStreamGraphRtpTx::create(RtpConfig* config)
{
    IMLOGI1("[create] state[%d]", mGraphState);

    if (config == NULL)
    {
        return RESULT_INVALID_PARAM;
    }

    VideoConfig* pConfig = reinterpret_cast<VideoConfig*>(config);

    if (pConfig->getVideoMode() == VideoConfig::VIDEO_MODE_PREVIEW)
    {
        return createPreviewMode(pConfig);
    }

    if (mConfig != NULL)
    {
        delete mConfig;
        mConfig = NULL;
    }

    mConfig = new VideoConfig(pConfig);

    char localIp[MAX_IP_LEN];
    uint32_t localPort = 0;
    ImsMediaNetworkUtil::getLocalIpPortFromSocket(mLocalFd, localIp, MAX_IP_LEN, localPort);
    RtpAddress localAddress(localIp, localPort);

    BaseNode* pNodeSource = new IVideoSourceNode(mCallback);
    pNodeSource->SetMediaType(IMS_MEDIA_VIDEO);
    pNodeSource->SetConfig(mConfig);
    AddNode(pNodeSource);

    BaseNode* pNodeRtpPayloadEncoder = new VideoRtpPayloadEncoderNode(mCallback);
    pNodeRtpPayloadEncoder->SetMediaType(IMS_MEDIA_VIDEO);
    pNodeRtpPayloadEncoder->SetConfig(mConfig);
    AddNode(pNodeRtpPayloadEncoder);
    pNodeSource->ConnectRearNode(pNodeRtpPayloadEncoder);

    BaseNode* pNodeRtpEncoder = new RtpEncoderNode(mCallback);
    pNodeRtpEncoder->SetMediaType(IMS_MEDIA_VIDEO);
    pNodeRtpEncoder->SetConfig(mConfig);
    ((RtpEncoderNode*)pNodeRtpEncoder)->SetLocalAddress(localAddress);
    AddNode(pNodeRtpEncoder);
    pNodeRtpPayloadEncoder->ConnectRearNode(pNodeRtpEncoder);

    BaseNode* pNodeSocketWriter = new SocketWriterNode(mCallback);
    pNodeSocketWriter->SetMediaType(IMS_MEDIA_VIDEO);
    ((SocketWriterNode*)pNodeSocketWriter)->SetLocalFd(mLocalFd);
    ((SocketWriterNode*)pNodeSocketWriter)->SetLocalAddress(localAddress);
    ((SocketWriterNode*)pNodeSocketWriter)->SetProtocolType(kProtocolRtp);
    pNodeSocketWriter->SetConfig(config);
    AddNode(pNodeSocketWriter);
    pNodeRtpEncoder->ConnectRearNode(pNodeSocketWriter);

    setState(StreamState::kStreamStateCreated);
    mVideoMode = pConfig->getVideoMode();
    return RESULT_SUCCESS;
}

ImsMediaResult VideoStreamGraphRtpTx::update(RtpConfig* config)
{
    IMLOGI2("[update] current mode[%d], state[%d]", mVideoMode, mGraphState);

    if (config == NULL)
    {
        return RESULT_INVALID_PARAM;
    }

    VideoConfig* pConfig = reinterpret_cast<VideoConfig*>(config);

    if (*reinterpret_cast<VideoConfig*>(mConfig) == *pConfig)
    {
        IMLOGD0("[update] no update");
        return RESULT_SUCCESS;
    }

    if (mGraphState == kStreamStateWaitSurface)
    {
        setState(StreamState::kStreamStateCreated);
    }

    if (mConfig != NULL)
    {
        delete mConfig;
        mConfig = NULL;
    }

    ImsMediaResult ret = RESULT_NOT_READY;

    if (pConfig->getVideoMode() != mVideoMode &&
            (mVideoMode == VideoConfig::VIDEO_MODE_PREVIEW ||
                    pConfig->getVideoMode() == VideoConfig::VIDEO_MODE_PREVIEW))
    {
        ret = stop();

        if (ret != RESULT_SUCCESS)
        {
            return ret;
        }

        /** delete nodes */
        deleteNodes();
        mSurface = NULL;

        /** create nodes */
        ret = create(pConfig);

        if (ret != RESULT_SUCCESS)
        {
            return ret;
        }

        return start();
    }

    mConfig = new VideoConfig(pConfig);

    if (mConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_NO_FLOW ||
            mConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_RECEIVE_ONLY ||
            mConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_INACTIVE)
    {
        IMLOGI0("[update] pause TX");
        return stop();
    }

    ret = RESULT_NOT_READY;

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
            (pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_SEND_ONLY ||
                    pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_SEND_RECEIVE))
    {
        IMLOGI0("[update] resume TX");
        return start();
    }

    return ret;
}

ImsMediaResult VideoStreamGraphRtpTx::start()
{
    IMLOGI2("[start] current mode[%d], state[%d]", mVideoMode, mGraphState);

    if (mConfig == NULL)
    {
        return RESULT_INVALID_PARAM;
    }

    VideoConfig* pConfig = reinterpret_cast<VideoConfig*>(mConfig);

    if (pConfig->getVideoMode() != VideoConfig::VIDEO_MODE_PREVIEW &&
            (pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_NO_FLOW ||
                    pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_RECEIVE_ONLY ||
                    pConfig->getMediaDirection() == RtpConfig::MEDIA_DIRECTION_INACTIVE))
    {
        IMLOGI1("[start] direction[%d] no need to start", pConfig->getMediaDirection());
        return RESULT_SUCCESS;
    }

    if (pConfig->getVideoMode() != VideoConfig::VIDEO_MODE_PAUSE_IMAGE && mSurface == NULL)
    {
        IMLOGI2("[start] direction[%d], mode[%d], surface is not ready, wait",
                pConfig->getMediaDirection(), pConfig->getVideoMode());
        setState(StreamState::kStreamStateWaitSurface);
        return RESULT_SUCCESS;
    }

    ImsMediaResult result = startNodes();

    if (result != RESULT_SUCCESS)
    {
        setState(StreamState::kStreamStateCreated);
        mCallback->SendEvent(kImsMediaEventNotifyError, result, kStreamModeRtpTx);
        return result;
    }

    setState(StreamState::kStreamStateRunning);
    mVideoMode = mConfig->getVideoMode();
    return RESULT_SUCCESS;
}

void VideoStreamGraphRtpTx::setSurface(ANativeWindow* surface)
{
    IMLOGD0("[setSurface]");

    if (surface != NULL)
    {
        mSurface = surface;

        BaseNode* node = findNode(kNodeIdVideoSource);

        if (node != NULL)
        {
            IVideoSourceNode* source = reinterpret_cast<IVideoSourceNode*>(node);
            source->UpdateSurface(surface);

            if (getState() == StreamState::kStreamStateWaitSurface)
            {
                setState(StreamState::kStreamStateCreated);
                start();
            }
        }
    }
}

ImsMediaResult VideoStreamGraphRtpTx::createPreviewMode(RtpConfig* config)
{
    if (config == NULL)
    {
        return RESULT_INVALID_PARAM;
    }

    if (mConfig != NULL)
    {
        delete mConfig;
        mConfig = NULL;
    }

    IMLOGI0("[createPreviewMode]");
    mConfig = new VideoConfig(reinterpret_cast<VideoConfig*>(config));
    BaseNode* pNodeSource = new IVideoSourceNode(mCallback);
    pNodeSource->SetMediaType(IMS_MEDIA_VIDEO);
    pNodeSource->SetConfig(mConfig);
    AddNode(pNodeSource);

    setState(StreamState::kStreamStateCreated);
    mVideoMode = VideoConfig::VIDEO_MODE_PREVIEW;
    return RESULT_SUCCESS;
}

bool VideoStreamGraphRtpTx::OnEvent(int32_t type, uint64_t param1, uint64_t param2)
{
    IMLOGI3("[OnEvent] type[%d], param1[%d], param2[%d]", type, param1, param2);

    switch (type)
    {
        case kRequestVideoCvoUpdate:
        {
            BaseNode* node = findNode(kNodeIdRtpEncoder);

            if (node != NULL)
            {
                RtpEncoderNode* pNode = reinterpret_cast<RtpEncoderNode*>(node);
                pNode->SetCvoExtension(param1, param2);
                return true;
            }
        }
        break;
        case kRequestVideoBitrateChange:
        case kRequestVideoIdrFrame:
        {
            BaseNode* node = findNode(kNodeIdVideoSource);

            if (node != NULL)
            {
                IVideoSourceNode* pNode = reinterpret_cast<IVideoSourceNode*>(node);
                pNode->OnEvent(type, param1, param2);
                return true;
            }
        }
        break;
        default:
            break;
    }

    return false;
}