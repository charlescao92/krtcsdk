#include <rtc_base/logging.h>

#include <exception>

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
    KRTCGlobal::Instance()->worker_thread()->PostTask([=]() {
        RTC_LOG(LS_INFO) << "KRTCPreview Start PostTask";

        KRTCError err = KRTCError::kNoErr;

        if (hwnd_ != 0) {
            local_renderer_ = VideoRenderer::Create(CONTROL_TYPE::PUSH, hwnd_, 1, 1);
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

            KRTCGlobal::Instance()->SetPreview(true);
        }
      
        if (KRTCGlobal::Instance()->engine_observer()) {
            if (err == KRTCError::kNoErr) {
                KRTCGlobal::Instance()->engine_observer()->OnPreviewSuccess();
            }
            else {
                KRTCGlobal::Instance()->engine_observer()->OnPreviewFailed(err);
            }
        }
    });
}

void KRTCPreview::Stop() {
    RTC_LOG(LS_INFO) << "KRTCPreview Stop call";
    KRTCGlobal::Instance()->worker_thread()->PostTask([=]() {
        RTC_LOG(LS_INFO) << "KRTCPreview Stop PostTask";

        KRTCGlobal::Instance()->SetPreview(false);

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

    });
}

void KRTCPreview::Destroy() {}

void KRTCPreview::OnFrame(const webrtc::VideoFrame& frame){
    if (KRTCGlobal::Instance()->engine_observer()) {
        try {
            rtc::scoped_refptr<webrtc::VideoFrameBuffer> vfb = frame.video_frame_buffer();
            if (!vfb || !vfb->GetI420()) {
                return;
            }

            int src_width = frame.width();
            int src_height = frame.height();

            int strideY = vfb->GetI420()->StrideY();
            int strideU = vfb->GetI420()->StrideU();
            int strideV = vfb->GetI420()->StrideV();

            int size = strideY * src_height + (strideU + strideV) * ((src_height + 1) / 2);
            std::shared_ptr<MediaFrame> media_frame = std::make_shared<MediaFrame>(size);
            media_frame->fmt.media_type = MainMediaType::kMainTypeVideo;
            media_frame->fmt.sub_fmt.video_fmt.type = SubMediaType::kSubTypeI420;
            media_frame->fmt.sub_fmt.video_fmt.width = src_width;
            media_frame->fmt.sub_fmt.video_fmt.height = src_height;
            media_frame->stride[0] = strideY;
            media_frame->stride[1] = strideU;
            media_frame->stride[2] = strideV;
            media_frame->data_len[0] = strideY * src_height;
            media_frame->data_len[1] = strideU * ((src_height + 1) / 2);
            media_frame->data_len[2] = strideV * ((src_height + 1) / 2);

            media_frame->data[0] = new char[size];
            media_frame->data[1] = media_frame->data[0] + media_frame->data_len[0];
            media_frame->data[2] = media_frame->data[1] + media_frame->data_len[1];
            if (vfb->GetI420()->DataY()) {
                memcpy(media_frame->data[0], vfb->GetI420()->DataY(), media_frame->data_len[0]);
            }
            if (vfb->GetI420()->DataU()) {
                memcpy(media_frame->data[1], vfb->GetI420()->DataU(), media_frame->data_len[1]);
            }
            if (vfb->GetI420()->DataV()) {
                memcpy(media_frame->data[2], vfb->GetI420()->DataV(), media_frame->data_len[2]);
            }

            KRTCGlobal::Instance()->engine_observer()->OnCapturePureVideoFrame(media_frame);

        }

        catch (std::exception e) {
                RTC_LOG(LS_ERROR) << "KRTCPreview error, " << e.what();
        }
    }

}

} // namespace krtc