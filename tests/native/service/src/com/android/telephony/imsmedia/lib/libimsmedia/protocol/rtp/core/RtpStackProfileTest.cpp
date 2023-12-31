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

#include <RtpStackProfile.h>
#include <gtest/gtest.h>

TEST(RtpStackProfileTest, TestConstructor)
{
    RtpStackProfile rtpStackProfile;

    // Check default value
    EXPECT_EQ(rtpStackProfile.getRtcpBandwidth(), RTP_DEF_RTCP_BW_SIZE);
    EXPECT_EQ(rtpStackProfile.getMtuSize(), RTP_CONF_MTU_SIZE);
    EXPECT_EQ(rtpStackProfile.getTermNumber(), RTP_ZERO);
}

TEST(RtpStackProfileTest, TestGetSets)
{
    RtpStackProfile rtpStackProfile;

    rtpStackProfile.setRtcpBandwidth(RTP_CONF_RTCP_BW_FRAC);
    EXPECT_EQ(rtpStackProfile.getRtcpBandwidth(), RTP_CONF_RTCP_BW_FRAC);

    rtpStackProfile.setMtuSize(RTP_DEF_MTU_SIZE);
    EXPECT_EQ(rtpStackProfile.getMtuSize(), RTP_DEF_MTU_SIZE);

    rtpStackProfile.setTermNumber(RTP_TWO_POWER_16);
    EXPECT_EQ(rtpStackProfile.getTermNumber(), RTP_TWO_POWER_16);
}