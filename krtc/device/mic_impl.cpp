#include "krtc/device/mic_impl.h"

#include <rtc_base/logging.h>
#include <rtc_base/task_utils/to_queued_task.h>
#include <api/audio/audio_frame.h>

#include "krtc/base/krtc_global.h"
#include "krtc/media/media_frame.h"

namespace krtc {

MicImpl::MicImpl(const char* mic_id) :
    mic_id_(mic_id),
    signaling_thread_(rtc::Thread::Current())
{
}

MicImpl::~MicImpl() {}

void MicImpl::Start() {
    RTC_LOG(LS_INFO) << "MicImpl Start call";

    KRTCGlobal::Instance()->worker_thread()->PostTask(webrtc::ToQueuedTask([=]() {

        RTC_LOG(LS_INFO) << "MicImpl Start PostTask";

        KRTCError err = KRTCError::kNoErr;

        do {
            // 1. 如果麦克风已经启动采集，直接停止
            if (has_start_) {
                RTC_LOG(LS_WARNING) << "mic already start, mic_id: " << mic_id_;
                break;
            }

            webrtc::AudioDeviceModule* audio_device =
                KRTCGlobal::Instance()->audio_device();
            // 2. 设置回调
            audio_device->RegisterAudioCallback(this);

            // 3. 检查系统是否存在麦克风设备
            int total = audio_device->RecordingDevices();
            if (total <= 0) {
                RTC_LOG(LS_WARNING) << "no audio device";
                err = KRTCError::kNoAudioDeviceErr;
                break;
            }

            // 4. 检查关联的mic_id是否能够在系统设备中找到
            int device_index = -1;
            for (int i = 0; i < total; ++i) {
                char name[128];
                char guid[128];
                audio_device->RecordingDeviceName(i, name, guid);
                if (0 == strcmp(guid, mic_id_.c_str())) {
                    device_index = i;
                    break;
                }
            }

            if (device_index <= -1) {
                RTC_LOG(LS_WARNING) << "audio device not found, mic_id: " << mic_id_;
                err = KRTCError::kAudioNotFoundErr;
                break;
            }

            // 5. 设置启用的麦克风设备
            if (audio_device->SetRecordingDevice(device_index)) {
                RTC_LOG(LS_WARNING) << "SetRecordingDevice failed, mic_id: " << mic_id_;
                err = KRTCError::kAudioSetRecordingDeviceErr;
                break;
            }

            // 6. 设置为立体声采集
            audio_device->SetStereoRecording(true);

            // 7. 初始化麦克风
            if (audio_device->InitRecording() || !audio_device->RecordingIsInitialized()) {
                RTC_LOG(LS_WARNING) << "InitRecording failed, mic_id: " << mic_id_;
                err = KRTCError::kAudioInitRecordingErr;
                break;
            }

            //audio_device ->StartPlayout();

            // 8. 启动麦克风采集
            if (audio_device->StartRecording()) {
                RTC_LOG(LS_WARNING) << "StartRecording failed, mic_id: " << mic_id_;
                err = KRTCError::kAudioStartRecordingErr;
                break;
            }

            has_start_ = true;

        } while (0);

        if (err == KRTCError::kNoErr) {
            if (KRTCGlobal::Instance()->engine_observer()) {
                KRTCGlobal::Instance()->engine_observer()->OnAudioSourceSuccess();
            }
        }
        else {
            if (KRTCGlobal::Instance()->engine_observer()) {
                KRTCGlobal::Instance()->engine_observer()->OnAudioSourceFailed(err);
            }
        }

    })); 
}

void MicImpl::Stop() {
    RTC_LOG(LS_INFO) << "MicImpl Stop call";
    KRTCGlobal::Instance()->worker_thread()->PostTask(webrtc::ToQueuedTask([=]() {
        RTC_LOG(LS_INFO) << "MicImpl Stop PostTask";
        if (!has_start_) {
            return;
        }

        has_start_ = false;

        webrtc::AudioDeviceModule* audio_device = KRTCGlobal::Instance()->audio_device(); 
        if (audio_device->RecordingIsInitialized()) {
            audio_device->StopRecording();
        }
    }));
}

void MicImpl::Destroy() {
    RTC_LOG(LS_INFO) << "MicImpl Destroy call";

    KRTCGlobal::Instance()->worker_thread()->PostTask(webrtc::ToQueuedTask([=]() {
        RTC_LOG(LS_INFO) << "MicImpl Destroy PostTask";
        delete this;
    }));
}

int32_t MicImpl::RecordedDataIsAvailable(const void* audioSamples,
    const size_t nSamples,  // 每个声道数包含的样本数
    const size_t nBytesPerSample, // 每个样本的字节数，是包括了通道数来计算的
    const size_t nChannels,
    const uint32_t samplesPerSec,
    const uint32_t totalDelayMS, 
    const int32_t clockDrift,
    const uint32_t currentMicLevel,
    const bool keyPressed,
    uint32_t& newMicLevel)
{
    int len = static_cast<int>(nSamples * nBytesPerSample);
    auto frame = std::make_shared<MediaFrame>(webrtc::AudioFrame::kMaxDataSizeBytes);
    frame->fmt.media_type = MainMediaType::kMainTypeAudio;
    frame->fmt.sub_fmt.audio_fmt.type = SubMediaType::kSubTypePcm;
    frame->fmt.sub_fmt.audio_fmt.nbytes_per_sample = nBytesPerSample;
    frame->fmt.sub_fmt.audio_fmt.samples_per_channel = nSamples;
    frame->fmt.sub_fmt.audio_fmt.channels = nChannels;
    frame->fmt.sub_fmt.audio_fmt.samples_per_sec = samplesPerSec;
    frame->fmt.sub_fmt.audio_fmt.total_delay_ms = totalDelayMS;
    frame->fmt.sub_fmt.audio_fmt.key_pressed = keyPressed;
    frame->data_len[0] = len;
    frame->data[0] = new char[frame->data_len[0]];
    memcpy(frame->data[0], audioSamples, len);

    // 计算时间戳，根据采样频率进行单调递增
    timestamp_ += nSamples;
    frame->ts = timestamp_;

    if (KRTCGlobal::Instance()->engine_observer()) {
        KRTCGlobal::Instance()->engine_observer()->OnPureAudioFrame(frame);
    }
    
    return 0;
}
} // namespace krtc
