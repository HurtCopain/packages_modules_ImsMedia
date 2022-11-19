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

#ifndef AUDIO_STREAM_GRAPH_H
#define AUDIO_STREAM_GRAPH_H

#include <ImsMediaDefine.h>
#include <BaseStreamGraph.h>
#include <AudioConfig.h>

class AudioStreamGraph : public BaseStreamGraph
{
public:
    AudioStreamGraph(BaseSessionCallback* callback, int localFd = 0) :
            BaseStreamGraph(callback, localFd),
            mConfig(NULL)
    {
    }
    virtual ~AudioStreamGraph()
    {
        if (mConfig != NULL)
        {
            delete mConfig;
            mConfig = NULL;
        }
    }

    virtual bool isSameGraph(RtpConfig* config)
    {
        if (config == NULL || mConfig == NULL)
        {
            return false;
        }

        return (mConfig->getRemoteAddress() == config->getRemoteAddress() &&
                mConfig->getRemotePort() == config->getRemotePort());
    }

protected:
    virtual ImsMediaResult create(RtpConfig* config) = 0;
    virtual ImsMediaResult update(RtpConfig* config) = 0;

    AudioConfig* mConfig;
};

#endif