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

#include <ImsMediaVideoSource.h>
#include <ImsMediaTrace.h>
#include <ImsMediaTimer.h>
#include <ImsMediaImageRotate.h>
#include <thread>
#include <list>
#include <time.h>

ImsMediaVideoSource::ImsMediaVideoSource()
{
    mCamera = NULL;
    mWindow = NULL;
    mCodec = NULL;
    mFormat = NULL;
    mRecordingSurface = NULL;
    mImageReaderSurface = NULL;
    mImageReader = NULL;
    mCodecType = -1;
    mVideoMode = -1;
    mCodecProfile = 0;
    mCodecLevel = 0;
    mCameraId = 0;
    mCameraZoom = 0;
    mWidth = 0;
    mHeight = 0;
    mFramerate = 0;
    mBitrate = 0;
    mIntraInterval = 1;
    mImagePath = android::String8("");
    mDeviceOrientation = -1;
    mTimestamp = 0;
    mPrevTimestamp = 0;
    mStopped = false;
}

ImsMediaVideoSource::~ImsMediaVideoSource() {}

void ImsMediaVideoSource::SetListener(IVideoSourceCallback* listener)
{
    std::lock_guard<std::mutex> guard(mMutex);
    mListener = listener;
}

void ImsMediaVideoSource::SetVideoMode(const int32_t mode)
{
    IMLOGD1("[SetVideoMode] mode[%d]", mode);
    mVideoMode = mode;
}

void ImsMediaVideoSource::SetCameraConfig(const uint32_t cameraId, const uint32_t cameraZoom)
{
    IMLOGD2("[SetCameraConfig] id[%d], zoom[%d]", cameraId, cameraZoom);
    mCameraId = cameraId;
    mCameraZoom = cameraZoom;
}

void ImsMediaVideoSource::SetImagePath(const android::String8 path)
{
    IMLOGD1("[SetImagePath] path[%s]", path.string());
    mImagePath = path;
}

void ImsMediaVideoSource::SetCodecConfig(int32_t codecType, const uint32_t profile,
        const uint32_t level, const uint32_t bitrate, const uint32_t framerate,
        const uint32_t interval)
{
    IMLOGD6("[SetCodecConfig] type[%d], profile[%d], level[%d], bitrate[%d], FPS[%d], interval[%d]",
            codecType, profile, level, bitrate, framerate, interval);
    mCodecType = codecType;
    mCodecProfile = profile;
    mCodecLevel = level;
    mBitrate = bitrate;
    mFramerate = framerate;
    mIntraInterval = interval;
}

void ImsMediaVideoSource::SetResolution(const uint32_t width, const uint32_t height)
{
    IMLOGD2("[SetResolution] width[%d], height[%d]", width, height);
    mWidth = width;
    mHeight = height;
}

void ImsMediaVideoSource::SetSurface(ANativeWindow* window)
{
    IMLOGD1("[SetSurface] surface[%p]", window);
    mWindow = window;
}

void ImsMediaVideoSource::SetDeviceOrientation(const uint32_t degree)
{
    IMLOGD1("[SetDeviceOrientation] degree[%d]", degree);

    if (mDeviceOrientation != degree)
    {
        if (mVideoMode == kVideoModeRecording)
        {
            int32_t facing = kCameraFacingFront;
            int32_t sensorOrientation = 0;
            int32_t rotateDegree = 0;

            if (mCamera != NULL)
            {
                mCamera->GetSensorOrientation(mCameraId, &facing, &sensorOrientation);
                IMLOGD2("[SetDeviceOrientation] camera facing[%d], sensorOrientation[%d]", facing,
                        sensorOrientation);
            }

            rotateDegree = 0;

            // assume device is always portrait
            if (mWidth > mHeight)
            {
                if (facing == kCameraFacingFront)
                {
                    sensorOrientation = (sensorOrientation + 180) % 360;
                }

                rotateDegree = sensorOrientation - degree;

                if (rotateDegree < 0)
                {
                    rotateDegree += 360;
                }
            }
            else
            {
                if (degree == 90 || degree == 270)
                {
                    rotateDegree = (degree + 180) % 360;
                }
                else
                {
                    rotateDegree = degree;
                }
            }

            if (mListener != NULL)
            {
                mListener->OnEvent(kVideoSourceEventUpdateOrientation, facing, rotateDegree);
            }
        }

        mDeviceOrientation = degree;
    }
}

bool ImsMediaVideoSource::Start()
{
    IMLOGD1("[Start], VideoMode[%d]", mVideoMode);
    mRecordingSurface = NULL;

    if (mVideoMode == kVideoModeRecording || mVideoMode == kVideoModePauseImage)
    {
        mFormat = AMediaFormat_new();
        AMediaFormat_setInt32(mFormat, AMEDIAFORMAT_KEY_WIDTH, mWidth);
        AMediaFormat_setInt32(mFormat, AMEDIAFORMAT_KEY_HEIGHT, mHeight);

        char kMimeType[128] = {'\0'};
        sprintf(kMimeType, "video/avc");

        if (mCodecType == kVideoCodecHevc)
        {
            sprintf(kMimeType, "video/hevc");
        }

        AMediaFormat_setString(mFormat, AMEDIAFORMAT_KEY_MIME, kMimeType);

        AMediaFormat_setInt32(mFormat, AMEDIAFORMAT_KEY_COLOR_FORMAT,
                0x00000015);  // COLOR_FormatYUV420SemiPlanar
        AMediaFormat_setInt32(mFormat, AMEDIAFORMAT_KEY_BIT_RATE, mBitrate * 1000);
        AMediaFormat_setInt32(mFormat, AMEDIAFORMAT_KEY_PROFILE, mCodecProfile);
        AMediaFormat_setInt32(mFormat, AMEDIAFORMAT_KEY_LEVEL, mCodecLevel);
        AMediaFormat_setInt32(mFormat, AMEDIAFORMAT_KEY_BITRATE_MODE,
                2);  // #2 : BITRATE_MODE_CBR
        AMediaFormat_setFloat(mFormat, AMEDIAFORMAT_KEY_FRAME_RATE, mFramerate);
        AMediaFormat_setInt32(mFormat, AMEDIAFORMAT_KEY_I_FRAME_INTERVAL, mIntraInterval);
        AMediaFormat_setInt32(mFormat, AMEDIAFORMAT_KEY_MAX_INPUT_SIZE, mWidth * mHeight * 10);

        mCodec = AMediaCodec_createEncoderByType(kMimeType);

        if (mCodec == NULL)
        {
            IMLOGE0("[Start] Unable to create encoder");
            return false;
        }

        media_status_t err = AMediaCodec_configure(
                mCodec, mFormat, NULL, NULL, AMEDIACODEC_CONFIGURE_FLAG_ENCODE);

        if (err != AMEDIA_OK)
        {
            IMLOGE1("[Start] configure error[%d]", err);
            AMediaCodec_delete(mCodec);
            mCodec = NULL;
            AMediaFormat_delete(mFormat);
            mFormat = NULL;
            return false;
        }

        if (mWidth > mHeight)  // Is Landscape Mode
        {
            err = AMediaCodec_createInputSurface(mCodec, &mRecordingSurface);

            if (err != AMEDIA_OK)
            {
                IMLOGE1("[Start] create input surface error[%d]", err);
                AMediaCodec_delete(mCodec);
                mCodec = NULL;
                AMediaFormat_delete(mFormat);
                mFormat = NULL;
                return false;
            }
        }
        else
        {
            mImageReaderSurface = CreateImageReader(mWidth, mHeight);

            if (mImageReaderSurface == NULL)
            {
                IMLOGE0("[Start] create image reader failed");
                return false;
            }
        }

        err = AMediaCodec_start(mCodec);

        if (err != AMEDIA_OK)
        {
            IMLOGE1("[Start] codec start[%d]", err);
            AMediaCodec_delete(mCodec);
            mCodec = NULL;
            AMediaFormat_delete(mFormat);
            mFormat = NULL;
            return false;
        }
    }

    if (mCameraId != -1 && (mVideoMode == kVideoModePreview || mVideoMode == kVideoModeRecording))
    {
        mCamera = ImsMediaCamera::getInstance();
        mCamera->Initialize();
        mCamera->SetCameraConfig(mCameraId, mCameraZoom, mFramerate);

        if (!mCamera->OpenCamera())
        {
            IMLOGE0("[Start] error open camera");
            AMediaCodec_delete(mCodec);
            mCodec = NULL;
            AMediaFormat_delete(mFormat);
            mFormat = NULL;
            return false;
        }

        ANativeWindow* recording = mRecordingSurface ? mRecordingSurface : mImageReaderSurface;

        if (mCamera->CreateSession(mWindow, recording) == false)
        {
            IMLOGE0("[Start] error create camera session");
            AMediaCodec_delete(mCodec);
            mCodec = NULL;
            AMediaFormat_delete(mFormat);
            mFormat = NULL;
            return false;
        }

        if (mCamera->StartSession(mVideoMode == kVideoModeRecording) == false)
        {
            IMLOGE0("[Start] error camera start");
            AMediaCodec_delete(mCodec);
            mCodec = NULL;
            AMediaFormat_delete(mFormat);
            mFormat = NULL;
            return false;
        }
    }
    else if (mVideoMode == kVideoModePauseImage)
    {
        mPauseImageSource.Initialize(mWidth, mHeight);
    }

    // start encoder output thread
    if (mCodec != NULL)
    {
        mStopped = false;
        std::thread t1(&ImsMediaVideoSource::processOutputBuffer, this);
        t1.detach();
    }

    mDeviceOrientation = -1;
    IMLOGD0("[Start] exit");
    return true;
}

void ImsMediaVideoSource::Stop()
{
    IMLOGD0("[Stop]");

    mMutex.lock();
    mStopped = true;
    mMutex.unlock();

    if (mCamera != NULL)
    {
        mCamera->StopSession();
    }

    IMLOGD0("[Stop] deinitialize camera");

    if (mCamera != NULL)
    {
        mCamera->DeleteSession();
        mCamera->DeInitialize();
        mCamera = NULL;
    }

    if (mCodec != NULL)
    {
        AMediaCodec_signalEndOfInputStream(mCodec);
        AMediaCodec_stop(mCodec);

        if (mRecordingSurface != NULL)
        {
            ANativeWindow_release(mRecordingSurface);
            mRecordingSurface = NULL;
        }

        AMediaCodec_delete(mCodec);
        mCodec = NULL;
    }

    if (mFormat != NULL)
    {
        AMediaFormat_delete(mFormat);
        mFormat = NULL;
    }

    if (mImageReader != NULL)
    {
        std::lock_guard<std::mutex> guard(mImageReaderMutex);
        AImageReader_delete(mImageReader);
        mImageReader = NULL;
        mImageReaderSurface = NULL;
    }

    mPauseImageSource.Uninitialize();
}

bool ImsMediaVideoSource::IsStopped()
{
    std::lock_guard<std::mutex> guard(mMutex);
    return mStopped;
}

void ImsMediaVideoSource::onCameraFrame(AImage* pImage)
{
    std::lock_guard<std::mutex> guard(mImageReaderMutex);

    if (mImageReader == NULL || pImage == NULL)
    {
        return;
    }

    auto index = AMediaCodec_dequeueInputBuffer(mCodec, 100000);

    if (index >= 0)
    {
        size_t buffCapacity = 0;
        uint8_t* encoderBuf = AMediaCodec_getInputBuffer(mCodec, index, &buffCapacity);

        if (!encoderBuf && !buffCapacity)
        {
            IMLOGE1("[onCameraFrame] returned null buffer pointer or buffCapacity[%d]",
                    buffCapacity);
            return;
        }

        int32_t width, height, ylen, uvlen;
        uint8_t *yPlane, *uvPlane;
        AImage_getWidth(pImage, &width);
        AImage_getHeight(pImage, &height);
        AImage_getPlaneData(pImage, 0, &yPlane, &ylen);
        AImage_getPlaneData(pImage, 1, &uvPlane, &uvlen);

        int32_t facing, sensorOrientation;
        mCamera->GetSensorOrientation(mCameraId, &facing, &sensorOrientation);

        switch (facing)
        {
            case ACAMERA_LENS_FACING_FRONT:
            {
                ImsMediaImageRotate::YUV420_SP_Rotate270(
                        encoderBuf, yPlane, uvPlane, width, height);
            }
            break;

            case ACAMERA_LENS_FACING_BACK:
            {
                ImsMediaImageRotate::YUV420_SP_Rotate90(encoderBuf, yPlane, uvPlane, width, height);
            }
            break;

            case ACAMERA_LENS_FACING_EXTERNAL:
            {
                uint32_t size = width * height;
                memcpy(encoderBuf, yPlane, size);
                memcpy(encoderBuf + size, uvPlane, size / 2);
            }
            break;
        }

        IMLOGD_PACKET1(IM_PACKET_LOG_VIDEO, "[onCameraFrame] queue buffer size[%d]", ylen + uvlen);

        AMediaCodec_queueInputBuffer(
                mCodec, index, 0, ylen + uvlen, ImsMediaTimer::GetTimeInMicroSeconds(), 0);
    }
    else
    {
        IMLOGE1("[onCameraFrame] dequeueInputBuffer returned index[%d]", index);
    }
}

void ImsMediaVideoSource::changeBitrate(const uint32_t bitrate)
{
    IMLOGD1("[changeBitrate] bitrate[%d]", bitrate);
    std::lock_guard<std::mutex> guard(mMutex);

    if (mStopped)
    {
        return;
    }

    AMediaFormat* params = AMediaFormat_new();
    AMediaFormat_setInt32(params, AMEDIAFORMAT_KEY_BIT_RATE, bitrate);
    media_status_t status = AMediaCodec_setParameters(mCodec, params);

    if (status != AMEDIA_OK)
    {
        IMLOGE1("[changeBitrate] error[%d]", status);
    }
}

void ImsMediaVideoSource::requestIdrFrame()
{
    IMLOGD0("[requestIdrFrame]");
    std::lock_guard<std::mutex> guard(mMutex);

    if (mStopped)
    {
        return;
    }

    AMediaFormat* params = AMediaFormat_new();
    AMediaFormat_setInt32(params, AMEDIACODEC_KEY_REQUEST_SYNC_FRAME, 0);
    media_status_t status = AMediaCodec_setParameters(mCodec, params);

    if (status != AMEDIA_OK)
    {
        IMLOGE1("[requestIdrFrame] error[%d]", status);
    }
}

void ImsMediaVideoSource::EncodePauseImage()
{
    auto index = AMediaCodec_dequeueInputBuffer(mCodec, 100000);
    if (index >= 0)
    {
        size_t buffCapacity = 0;
        uint8_t* encoderBuf = AMediaCodec_getInputBuffer(mCodec, index, &buffCapacity);
        if (!encoderBuf && !buffCapacity)
        {
            IMLOGE1("[EncodePauseImage] returned null buffer pointer or buffCapacity[%d]",
                    buffCapacity);
            return;
        }
        size_t len = mPauseImageSource.GetYuvImage(encoderBuf, buffCapacity);
        AMediaCodec_queueInputBuffer(
                mCodec, index, 0, len, ImsMediaTimer::GetTimeInMicroSeconds(), 0);
    }
    else
    {
        IMLOGE1("[EncodePauseImage] dequeueInputBuffer returned index[%d]", index);
    }
}

void ImsMediaVideoSource::processOutputBuffer()
{
    static int kTimeout = 100000;  // be responsive on signal
    uint32_t nextTime = ImsMediaTimer::GetTimeInMilliSeconds();
    uint32_t timeInterval = 66;
    uint32_t timeDiff = 0;

    if (mFramerate != 0)
    {
        timeInterval = 1000 / mFramerate;
    }

    IMLOGD2("[processOutputBuffer] interval[%d] mCameraId[%d]", timeInterval, mCameraId);

    for (;;)
    {
        uint32_t nCurrTime;
        mMutex.lock();

        if (mStopped)
        {
            IMLOGD0("[processOutputBuffer] terminated");
            mMutex.unlock();
            break;
        }

        mMutex.unlock();
        if (mVideoMode == kVideoModePauseImage)
        {
            EncodePauseImage();
        }

        AMediaCodecBufferInfo info;
        auto index = AMediaCodec_dequeueOutputBuffer(mCodec, &info, kTimeout);

        if (index >= 0)
        {
            IMLOGD_PACKET5(IM_PACKET_LOG_VIDEO,
                    "[processOutputBuffer] index[%d], size[%d], offset[%d], time[%ld], flags[%d]",
                    index, info.size, info.offset, info.presentationTimeUs, info.flags);

            if (info.size > 0)
            {
                size_t buffCapacity;
                uint8_t* buf = AMediaCodec_getOutputBuffer(mCodec, index, &buffCapacity);

                if (buf != NULL && buffCapacity > 0)
                {
                    if (mListener != NULL)
                    {
                        mListener->OnUplinkEvent(
                                buf + info.offset, info.size, info.presentationTimeUs, info.flags);
                    }
                }
            }

            if (!IsStopped())
            {
                AMediaCodec_releaseOutputBuffer(mCodec, index, false);
            }
        }
        else if (index == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED)
        {
            IMLOGD0("[processOutputBuffer] Encoder output buffer changed");
        }
        else if (index == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED)
        {
            if (mFormat != NULL)
            {
                AMediaFormat_delete(mFormat);
            }

            mFormat = AMediaCodec_getOutputFormat(mCodec);
            IMLOGD1("[processOutputBuffer] Encoder format changed, format[%s]",
                    AMediaFormat_toString(mFormat));
        }
        else if (index == AMEDIACODEC_INFO_TRY_AGAIN_LATER)
        {
            IMLOGD_PACKET0(IM_PACKET_LOG_VIDEO, "[processOutputBuffer] no output buffer");
        }
        else
        {
            IMLOGD1("[processOutputBuffer] unexpected index[%d]", index);
        }

        nextTime += timeInterval;
        nCurrTime = ImsMediaTimer::GetTimeInMilliSeconds();

        if (nextTime > nCurrTime)
        {
            timeDiff = nextTime - nCurrTime;
            IMLOGD_PACKET1(IM_PACKET_LOG_VIDEO, "[processOutputBuffer] timeDiff[%u]", timeDiff);
            ImsMediaTimer::Sleep(timeDiff);
        }
    }

    IMLOGD0("[processOutputBuffer] exit");
}

static void ImageCallback(void* context, AImageReader* reader)
{
    if (context != NULL)
    {
        ImsMediaVideoSource* pVideoSource = static_cast<ImsMediaVideoSource*>(context);

        if (pVideoSource->IsStopped())
        {
            return;
        }

        AImage* image = nullptr;
        auto status = AImageReader_acquireNextImage(reader, &image);

        if (status != AMEDIA_OK)
        {
            return;
        }

        pVideoSource->onCameraFrame(image);
        AImage_delete(image);
    }
}

ANativeWindow* ImsMediaVideoSource::CreateImageReader(int width, int height)
{
    media_status_t status =
            AImageReader_new(width, height, AIMAGE_FORMAT_YUV_420_888, 2, &mImageReader);

    if (status != AMEDIA_OK)
    {
        return NULL;
    }

    AImageReader_ImageListener listener{
            .context = this,
            .onImageAvailable = ImageCallback,
    };

    AImageReader_setImageListener(mImageReader, &listener);

    ANativeWindow* nativeWindow;
    AImageReader_getWindow(mImageReader, &nativeWindow);
    return nativeWindow;
}