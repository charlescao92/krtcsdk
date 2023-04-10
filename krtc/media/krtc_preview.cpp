#include <rtc_base/logging.h>
#include <rtc_base/task_utils/to_queued_task.h>

#include "krtc/media/default.h"
#include "krtc/media/krtc_preview.h"
#include "krtc/base/krtc_global.h"
#include "krtc/base/krtc_thread.h"
#include "krtc/render/video_renderer.h"
#include "krtc/device/vcm_capturer.h"
#include "krtc/device/desktop_capturer.h"
#include "krtc/media/media_frame.h"

namespace krtc {

KRTCPreview::KRTCPreview(const int& hwnd):
    hwnd_(hwnd),
    current_thread_(std::make_unique<KRTCThread>(rtc::Thread::Current()))
{
}

KRTCPreview::~KRTCPreview() = default;


void KRTCPreview::Start() {
    RTC_LOG(LS_INFO) << "KRTCPreview Start call";
    KRTCGlobal::Instance()->worker_thread()->PostTask(webrtc::ToQueuedTask([=]() {
        RTC_LOG(LS_INFO) << "KRTCPreview Start PostTask";

        KRTCError err = KRTCError::kNoErr;

        if (hwnd_ != 0) {
            local_renderer_ = VideoRenderer::Create(hwnd_, 1, 1);
            if (local_renderer_) {
                webrtc::VideoTrackSource* video_source = KRTCGlobal::Instance()->current_video_source();
                if (video_source) {
                    video_source->AddOrUpdateSink(local_renderer_.get(), rtc::VideoSinkWants());
                }
                else {
                    err = KRTCError::kPreviewNoVideoSourceErr;
                }

            }
            else {
                err = KRTCError::kPreviewCreateRenderDeviceErr;
            }
        }
        else {
            webrtc::VideoTrackSource* video_source = KRTCGlobal::Instance()->current_video_source();
            if (video_source) {
                video_source->AddOrUpdateSink(this, rtc::VideoSinkWants());
            }
        }
      
        if (KRTCGlobal::Instance()->engine_observer()) {
            if (err == KRTCError::kNoErr) {
                KRTCGlobal::Instance()->engine_observer()->OnPreviewSuccess();
            }
            else {
                KRTCGlobal::Instance()->engine_observer()->OnPreviewFailed(err);
            }
        }
    }));
}

void KRTCPreview::Stop() {
    RTC_LOG(LS_INFO) << "KRTCPreview Stop call";
    KRTCGlobal::Instance()->worker_thread()->PostTask(webrtc::ToQueuedTask([=]() {
        RTC_LOG(LS_INFO) << "KRTCPreview Stop PostTask";

        webrtc::VideoTrackSource* video_source = KRTCGlobal::Instance()->current_video_source();
        if (!video_source) {
            return;
        }

        if (local_renderer_) {
            video_source->RemoveSink(local_renderer_.get());
        }
        else {
            video_source->RemoveSink(this);
        }

    }));
}

void KRTCPreview::Destroy()
{
}

void KRTCPreview::OnFrame(const webrtc::VideoFrame& frame)
{
    if (hwnd_ != 0) {
        return;
    }

    if (KRTCGlobal::Instance()->engine_observer()) {
        int src_width = frame.width();
        int src_height = frame.height();

        // 获取YUV数据的每一行的长度
        int strideY = frame.video_frame_buffer()->GetI420()->StrideY();
        int strideU = frame.video_frame_buffer()->GetI420()->StrideU();
        int strideV = frame.video_frame_buffer()->GetI420()->StrideV();

        KRTCGlobal::Instance()->engine_observer()->OnPureVideoFrame(
            src_width, src_height,
            frame.video_frame_buffer()->GetI420()->DataY(),
            frame.video_frame_buffer()->GetI420()->DataU(),
            frame.video_frame_buffer()->GetI420()->DataV(),
            strideY, strideU, strideV);
    }
}

} // namespace krtc