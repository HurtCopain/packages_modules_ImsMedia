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

#ifndef IMSMEDIA_AUDIO_SOURCE_INCLUDED
#define IMSMEDIA_AUDIO_SOURCE_INCLUDED

#include <ImsMediaDefine.h>
#include <ImsMediaAudioDefine.h>
#include <mutex>
#include <IImsMediaThread.h>
#include <ImsMediaCondition.h>
#include <aaudio/AAudio.h>
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>

using android::sp;

typedef void (*AudioUplinkCB)(
        void* pClient, uint8_t* pBitstream, uint32_t nSize, int64_t pstUsec, uint32_t flag);

class ImsMediaAudioSource : public IImsMediaThread
{
public:
    std::mutex mMutexUplink;
    AAudioStream* mAudioStream;
    AMediaCodec* mCodec;
    AMediaFormat* mFormat;
    AudioUplinkCB mUplinkCB;
    void* mUplinkCBClient;
    int32_t mCodecType;
    uint32_t mMode;
    uint32_t mPtime;
    uint32_t mSamplingRate;
    uint32_t mBufferSize;

private:
    void openAudioStream();
    void restartAudioStream();
    static void audioErrorCallback(AAudioStream* stream, void* userData, aaudio_result_t error);

public:
    ImsMediaAudioSource();
    virtual ~ImsMediaAudioSource();
    void SetUplinkCallback(void* pClient, AudioUplinkCB pDnlinkCB);
    void SetCodec(int32_t type);
    void SetCodecMode(uint32_t mode);
    void SetPtime(uint32_t time);
    bool Start();
    void Stop();
    bool ProcessCMR(uint32_t mode);
    void queueInputBuffer(int16_t* buffer, uint32_t size);
    void processOutputBuffer();
    virtual void* run();
};

#endif
