#include "krtc/device/vcm_capturer.h"

#include <stdint.h>

#include <memory>

#include <modules/video_capture/video_capture_factory.h>
#include <rtc_base/checks.h>
#include <rtc_base/logging.h>
#include <rtc_base/task_utils/to_queued_task.h>
#include <api/video/i420_buffer.h>

#include "krtc/base/krtc_global.h"
#include "krtc/media/media_frame.h"

namespace krtc {

std::unique_ptr<VcmCapturer> VcmCapturer::Create(const std::string& cam_id,  size_t width, size_t height,
                                                size_t target_fps)
{
    std::unique_ptr<VcmCapturer> vcm_capturer(new VcmCapturer(cam_id, width, height, target_fps));
    return std::move(vcm_capturer);
}

VcmCapturer::VcmCapturer(const std::string& cam_id, size_t width, size_t height, size_t target_fps) :
    cam_id_(cam_id),
    vcm_(nullptr),
    signaling_thread_(rtc::Thread::Current()),
    device_info_(KRTCGlobal::Instance()->video_device_info())
{
    capability_.width = static_cast<int32_t>(width);
    capability_.height = static_cast<int32_t>(height);
    capability_.maxFPS = static_cast<int32_t>(target_fps);
    capability_.videoType = webrtc::VideoType::kI420;
}

void VcmCapturer::Start()
{
    RTC_LOG(LS_INFO) << "CamImpl Start call";
    signaling_thread_->PostTask(webrtc::ToQueuedTask([=] {
        RTC_LOG(LS_INFO) << "CamImpl Start PostTask";

        KRTCError err = KRTCError::kNoErr;
        do {
            if (has_start_) {
                RTC_LOG(LS_WARNING) << "CamImpl already start, ignore";
                break;
            }

            vcm_ = webrtc::VideoCaptureFactory::Create(cam_id_.c_str());
            if (!vcm_) {
                err = KRTCError::kVideoCreateCaptureErr;
                break;
            }

            if (device_info_->NumberOfCapabilities(cam_id_.c_str()) <= 0) {
                err = KRTCError::kVideoNoCapabilitiesErr;
                RTC_LOG(LS_WARNING) << "CamImpl no capabilities";
                break;
            }

            webrtc::VideoCaptureCapability best_cap;

            if (device_info_->GetBestMatchedCapability(cam_id_.c_str(),
                capability_, best_cap) < 0)
            {
                err = KRTCError::kVideoNoBestCapabilitiesErr;
                RTC_LOG(LS_WARNING) << "CamImpl no best capabilities";
                break;
            }

            vcm_->RegisterCaptureDataCallback(this);

            if (vcm_->StartCapture(best_cap) < 0) {
                err = KRTCError::kVideoStartCaptureErr;
                RTC_LOG(LS_WARNING) << "CamImpl video StartCapture error";
                break;
            }

            if (vcm_->CaptureStarted()) {
                has_start_ = true;
            }

        } while (0);

        if (err != KRTCError::kNoErr) {
            if (KRTCGlobal::Instance()->engine_observer()) {
                KRTCGlobal::Instance()->engine_observer()->OnVideoSourceFailed(err);
            }
        }
        else {
            if (KRTCGlobal::Instance()->engine_observer()) {
                KRTCGlobal::Instance()->engine_observer()->OnVideoSourceSuccess();
            }
        }
    }));
}

void VcmCapturer::Stop() {
    if (has_start_) {
        Destroy();
        has_start_ = false;
    }
}

void VcmCapturer::Destroy() {
    if (!has_start_) {
        return;
    }

    if (!vcm_)
        return;

    vcm_->StopCapture();
    vcm_->DeRegisterCaptureDataCallback();
    vcm_ = nullptr;
}

VcmCapturer::~VcmCapturer() {
    Destroy();
}

void VcmCapturer::OnFrame(const webrtc::VideoFrame& frame) {
    VideoCapturer::OnFrame(frame);
}

webrtc::VideoFrame VcmFramePreprocessor::Preprocess(const webrtc::VideoFrame& frame)
{
    if (!KRTCGlobal::Instance()->engine_observer()) {
        return frame;
    }

    rtc::scoped_refptr<webrtc::VideoFrameBuffer> vfb = frame.video_frame_buffer();
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

    // 拿到每个平面数组的指针，然后拷贝数据到平面数组里面
    media_frame->data[0] = new char[media_frame->data_len[0]];
    media_frame->data[1] = new char[media_frame->data_len[1]];
    media_frame->data[2] = new char[media_frame->data_len[2]];
    memcpy(media_frame->data[0], vfb->GetI420()->DataY(), media_frame->data_len[0]);
    memcpy(media_frame->data[1], vfb->GetI420()->DataU(), media_frame->data_len[1]);
    memcpy(media_frame->data[2], vfb->GetI420()->DataV(), media_frame->data_len[2]);

    MediaFrame *preprocessed_frame = 
        KRTCGlobal::Instance()->engine_observer()->OnPreprocessVideoFrame(media_frame.get());

    rtc::scoped_refptr<webrtc::I420Buffer> yuv_buffer(new rtc::RefCountedObject<webrtc::I420Buffer>(src_width, src_height));
    memcpy((char*)yuv_buffer->MutableDataY(), preprocessed_frame->data[0], preprocessed_frame->data_len[0]);
    memcpy((char*)yuv_buffer->MutableDataU(), preprocessed_frame->data[1], preprocessed_frame->data_len[1]);
    memcpy((char*)yuv_buffer->MutableDataV(), preprocessed_frame->data[2], preprocessed_frame->data_len[2]);

    webrtc::VideoFrame video_frame(yuv_buffer, 0, 0, webrtc::kVideoRotation_0);
    video_frame.set_timestamp_us(rtc::TimeMicros()); // 设置为当前时间
    return video_frame;
}

} // namespace krtc
