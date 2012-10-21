/*
<<<<<<< HEAD
 * Copyright (C) 2007 The Android Open Source Project
=======
 * Copyright (C) 2012 The Android Open Source Project
>>>>>>> 837cfa0... add overlay and some changes for camerahal
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

<<<<<<< HEAD
=======

//#define LOG_NDEBUG 0
>>>>>>> 837cfa0... add overlay and some changes for camerahal
#define LOG_TAG "Overlay"

#include <binder/IMemory.h>
#include <binder/Parcel.h>
#include <utils/Errors.h>
#include <binder/MemoryHeapBase.h>
<<<<<<< HEAD
=======
#include <cutils/ashmem.h>
>>>>>>> 837cfa0... add overlay and some changes for camerahal

#include <ui/Overlay.h>

namespace android {

<<<<<<< HEAD
Overlay::Overlay(overlay_set_fd_hook set_fd,
        overlay_set_crop_hook set_crop,
        overlay_queue_buffer_hook queue_buffer,
        void *data)
    : mStatus(NO_INIT)
{
    set_fd_hook = set_fd;
    set_crop_hook = set_crop;
    queue_buffer_hook = queue_buffer;
    hook_data = data;
=======
int Overlay::getBppFromFormat(const Format format)
{
    switch(format) {
    case FORMAT_RGBA8888:
        return 32;
    case FORMAT_RGB565:
    case FORMAT_YUV422I:
    case FORMAT_YUV422SP:
        return 16;
    case FORMAT_YUV420SP:
    case FORMAT_YUV420P:
        return 12;
    default:
        LOGW("%s: unhandled color format %d", __FUNCTION__, format);
    }
    return 32;
}

Overlay::Format Overlay::getFormatFromString(const char* name)
{
    if (strcmp(name, "yuv422sp") == 0) {
        return FORMAT_YUV422SP;
    } else if (strcmp(name, "yuv420sp") == 0) {
        return FORMAT_YUV420SP;
    } else if (strcmp(name, "yuv422i-yuyv") == 0) {
        return FORMAT_YUV422I;
    } else if (strcmp(name, "yuv420p") == 0) {
        return FORMAT_YUV420P;
    } else if (strcmp(name, "rgb565") == 0) {
        return FORMAT_RGB565;
    } else if (strcmp(name, "rgba8888") == 0) {
        return FORMAT_RGBA8888;
    }
    LOGW("%s: unhandled color format %s", __FUNCTION__, name);
    return FORMAT_UNKNOWN;
}

Overlay::Overlay(uint32_t width, uint32_t height, Format format, QueueBufferHook queueBufferHook, void *data) :
    mQueueBufferHook(queueBufferHook),
    mHookData(data),
    mNumFreeBuffers(0),
    mStatus(NO_INIT),
    mWidth(width),
    mHeight(height),
    mFormat(format)
{
    LOGD("%s: Init overlay", __FUNCTION__);

    int bpp = getBppFromFormat(format);
    /* round up to next multiple of 8 */
    if (bpp & 7) {
        bpp = (bpp & ~7) + 8;
    }

    const int requiredMem = width * height * bpp;
    const int bufferSize = (requiredMem + PAGE_SIZE - 1) & (~(PAGE_SIZE - 1));

    int fd = ashmem_create_region("Overlay_buffer_region", NUM_BUFFERS * bufferSize);
    if (fd < 0) {
        LOGE("%s: Cannot create ashmem region", __FUNCTION__);
        return;
    }

    LOGV("%s: allocated ashmem region for %d buffers of size %d", __FUNCTION__, NUM_BUFFERS, bufferSize);

    for (uint32_t i = 0; i < NUM_BUFFERS; i++) {
        mBuffers[i].fd = fd;
        mBuffers[i].length = bufferSize;
        mBuffers[i].offset = bufferSize * i;
        LOGV("%s: mBuffers[%d].offset = 0x%x", __FUNCTION__, i, mBuffers[i].offset);
        mBuffers[i].ptr = mmap(NULL, bufferSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, bufferSize * i);
        if (mBuffers[i].ptr == MAP_FAILED) {
            LOGE("%s: Failed to mmap buffer %d", __FUNCTION__, i);
            mBuffers[i].ptr = NULL;
            continue;
        }
        mQueued[i] = false;
    }

    pthread_mutex_init(&mQueueMutex, NULL);

    LOGD("%s: Init overlay complete", __FUNCTION__);

>>>>>>> 837cfa0... add overlay and some changes for camerahal
    mStatus = NO_ERROR;
}

Overlay::~Overlay() {
}

<<<<<<< HEAD
status_t Overlay::dequeueBuffer(void** buffer)
{
    return mStatus;
}

status_t Overlay::queueBuffer(void* buffer)
{
    if (queue_buffer_hook)
        queue_buffer_hook(hook_data, buffer);
=======
status_t Overlay::dequeueBuffer(overlay_buffer_t* buffer)
{
    LOGV("%s", __FUNCTION__);
    int rv = NO_ERROR;

    pthread_mutex_lock(&mQueueMutex);

    if (mNumFreeBuffers < NUM_MIN_FREE_BUFFERS) {
        LOGV("%s: No free buffers", __FUNCTION__);
        rv = NO_MEMORY;
    } else {
        int index = -1;

        for (uint32_t i = 0; i < NUM_BUFFERS; i++) {
            if (mQueued[i]) {
                mQueued[i] = false;
                index = i;
                break;
            }
        }

        if (index >= 0) {
            int *intBuffer = (int *) buffer;
            *intBuffer = index;
            mNumFreeBuffers--;
            LOGV("%s: dequeued buffer %d", __FUNCTION__, index);
        } else {
            LOGE("%s: inconsistent queue state", __FUNCTION__);
            rv = NO_MEMORY;
        }
    }

    pthread_mutex_unlock(&mQueueMutex);
    return rv;
}

status_t Overlay::queueBuffer(overlay_buffer_t buffer)
{
    uint32_t index = (uint32_t) buffer;
    int rv;

    LOGV("%s: %d", __FUNCTION__, index);
    if (index > NUM_BUFFERS) {
        LOGE("%s: invalid buffer index %d", __FUNCTION__, index);
        return INVALID_OPERATION;
    }

    if (mQueueBufferHook) {
        mQueueBufferHook(mHookData, mBuffers[index].ptr, mBuffers[index].length);
    }

    pthread_mutex_lock(&mQueueMutex);

    if (mNumFreeBuffers < NUM_BUFFERS) {
        mNumFreeBuffers++;
        mQueued[index] = true;
        rv = NO_ERROR;
    } else {
        LOGW("%s: Attempt to queue more buffers than we have", __FUNCTION__);
        rv = INVALID_OPERATION;
    }

    pthread_mutex_unlock(&mQueueMutex);

>>>>>>> 837cfa0... add overlay and some changes for camerahal
    return mStatus;
}

status_t Overlay::resizeInput(uint32_t width, uint32_t height)
{
<<<<<<< HEAD
=======
    LOGW("%s: %d, %d", __FUNCTION__, width, height);
>>>>>>> 837cfa0... add overlay and some changes for camerahal
    return mStatus;
}

status_t Overlay::setParameter(int param, int value)
{
<<<<<<< HEAD
=======
    LOGW("%s: %d, %d", __FUNCTION__, param, value);
>>>>>>> 837cfa0... add overlay and some changes for camerahal
    return mStatus;
}

status_t Overlay::setCrop(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
<<<<<<< HEAD
    if (set_crop_hook)
        set_crop_hook(hook_data, x, y, w, h);
=======
    LOGD("%s: x=%d, y=%d, w=%d, h=%d", __FUNCTION__, x, y, w, h);
>>>>>>> 837cfa0... add overlay and some changes for camerahal
    return mStatus;
}

status_t Overlay::getCrop(uint32_t* x, uint32_t* y, uint32_t* w, uint32_t* h)
{
<<<<<<< HEAD
=======
    LOGW("%s", __FUNCTION__);
>>>>>>> 837cfa0... add overlay and some changes for camerahal
    return mStatus;
}

status_t Overlay::setFd(int fd)
{
<<<<<<< HEAD
    if (set_fd_hook)
        set_fd_hook(hook_data, fd);
=======
    LOGW("%s: fd=%d", __FUNCTION__, fd);
>>>>>>> 837cfa0... add overlay and some changes for camerahal
    return mStatus;
}

int32_t Overlay::getBufferCount() const
{
<<<<<<< HEAD
    return 0;
}

void* Overlay::getBufferAddress(void* buffer)
{
    return 0;
}

void Overlay::destroy() {  
}

status_t Overlay::getStatus() const {
    return mStatus;
}

void* Overlay::getHandleRef() const {
    return 0;
}

uint32_t Overlay::getWidth() const {
    return 0;
}

uint32_t Overlay::getHeight() const {
    return 0;
}

int32_t Overlay::getFormat() const {
    return 0;
}

int32_t Overlay::getWidthStride() const {
    return 0;
}

int32_t Overlay::getHeightStride() const {
    return 0;
}

// ----------------------------------------------------------------------------

=======
    LOGV("%s: %d", __FUNCTION__, NUM_BUFFERS);
    return NUM_BUFFERS;
}

void* Overlay::getBufferAddress(overlay_buffer_t buffer)
{
    uint32_t index = (uint32_t) buffer;

    LOGD("%s: %d", __FUNCTION__, index);
    if (index >= NUM_BUFFERS) {
        index = index % NUM_BUFFERS;
    }

    //LOGD("%s: fd=%d, length=%d. offset=%d, ptr=%p", __FUNCTION__, mBuffers[index].fd,
    //        mBuffers[index].length, mBuffers[index].offset, mBuffers[index].ptr);

    return &mBuffers[index];
}

void Overlay::destroy()
{
    int fd = 0;

    LOGV("%s", __FUNCTION__);

    for (uint32_t i = 0; i < NUM_BUFFERS; i++) {
        if (mBuffers[i].ptr != NULL && munmap(mBuffers[i].ptr, mBuffers[i].length) < 0) {
            LOGW("%s: unmap of buffer %d failed", __FUNCTION__, i);
        }
        if (mBuffers[i].fd > 0) {
            fd = mBuffers[i].fd;
        }
    }
    if (fd > 0) {
        close(fd);
    }

    pthread_mutex_destroy(&mQueueMutex);
}

status_t Overlay::getStatus() const
{
    LOGV("%s", __FUNCTION__);
    return mStatus;
}

overlay_handle_t Overlay::getHandleRef() const
{
    LOGW("%s", __FUNCTION__);
    return 0;
}

uint32_t Overlay::getWidth() const
{
    LOGV("%s", __FUNCTION__);
    return mWidth;
}

uint32_t Overlay::getHeight() const
{
    LOGV("%s", __FUNCTION__);
    return mHeight;
}

int32_t Overlay::getFormat() const
{
    LOGV("%s", __FUNCTION__);
    return mFormat;
}

int32_t Overlay::getWidthStride() const
{
    LOGW("%s", __FUNCTION__);
    return mWidth;
}

int32_t Overlay::getHeightStride() const
{
    LOGW("%s", __FUNCTION__);
    return mHeight;
}

>>>>>>> 837cfa0... add overlay and some changes for camerahal
}; // namespace android
