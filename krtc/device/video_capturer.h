/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef KRTC_DEVICE_VIDEO_CAPTURER_H_
#define KRTC_DEVICE_VIDEO_CAPTURER_H_

#include <stddef.h>

#include <memory>
#include <atomic>

#include <api/video/video_frame.h>
#include <api/video/video_source_interface.h>
#include <media/base/video_adapter.h>
#include <media/base/video_broadcaster.h>
#include <rtc_base/synchronization/mutex.h>

namespace krtc {

class VideoCapturer : public rtc::VideoSourceInterface<webrtc::VideoFrame> {
public:
    class FramePreprocessor {
    public:
        virtual ~FramePreprocessor() = default;

        virtual webrtc::VideoFrame Preprocess(const webrtc::VideoFrame& frame) = 0;
    };

    ~VideoCapturer() override;

    void AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
        const rtc::VideoSinkWants& wants) override;
    void RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) override;
    void SetFramePreprocessor(std::unique_ptr<FramePreprocessor> preprocessor) {
        webrtc::MutexLock lock(&lock_);
        preprocessor_ = std::move(preprocessor);
    }

protected:
    void OnFrame(const webrtc::VideoFrame& frame);
    
    rtc::VideoSinkWants GetSinkWants();

private:
    void CalcFps(const webrtc::VideoFrame& frame);

    void UpdateVideoAdapter();
    webrtc::VideoFrame MaybePreprocess(const webrtc::VideoFrame& frame);

    webrtc::Mutex lock_;
    std::unique_ptr<FramePreprocessor> preprocessor_ RTC_GUARDED_BY(lock_);
    rtc::VideoBroadcaster broadcaster_;
    cricket::VideoAdapter video_adapter_;

    std::atomic<int> fps_{ 0 };
    std::atomic<int64_t> last_frame_ts_{ 0 };
    std::atomic<int64_t> start_time_{ 0 };
};

}  // namespace krtc

#endif  // KRTC_DEVICE_VIDEO_CAPTURER_H_
