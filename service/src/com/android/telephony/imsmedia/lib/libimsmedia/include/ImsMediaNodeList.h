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

#ifndef IMS_MEDIA_NODE_LIST_H
#define IMS_MEDIA_NODE_LIST_H

#include <BaseNodeID.h>
#include <SocketWriterNode.h>
#include <SocketReaderNode.h>
#include <RtpEncoderNode.h>
#include <RtpDecoderNode.h>
#include <RtcpEncoderNode.h>
#include <RtcpDecoderNode.h>
#include <DtmfEncoderNode.h>
#include <DtmfSenderNode.h>
#include <IAudioPlayerNode.h>
#include <IAudioSourceNode.h>
#include <AudioRtpPayloadDecoderNode.h>
#include <AudioRtpPayloadEncoderNode.h>

typedef BaseNode* (*fn_GetNodeInstance)();
typedef void (*fn_ReleaseNodeInstance)(BaseNode* pNode);

typedef struct _tNodeListEntry {
    BaseNodeID NodeID;
    const char* NodeName;
    fn_GetNodeInstance         GetInstance;
    fn_ReleaseNodeInstance     DeleteInstance;
} tNodeListEntry;

#endif