#include "audio_device_data_observer.h"
#include "krtc/base/krtc_global.h"
#include "krtc/media/media_frame.h"

namespace krtc {

    void ADMDataObserver::OnCaptureData(const void* audio_samples,
        const size_t num_samples,
        const size_t bytes_per_sample,
        const size_t num_channels,
        const uint32_t samples_per_sec) {
      /*/  RTC_LOG(LS_INFO) << "音频采集数据，num_samples:" << num_samples
            << " bytes_per_sample:" << bytes_per_sample
            << " num_channels:" << num_channels
            << "samples_per_sec:" << samples_per_sec;*/

        int len = static_cast<int>(num_samples * bytes_per_sample);
        auto frame = std::make_shared<MediaFrame>(len); //webrtc::AudioFrame::kMaxDataSizeBytes
        frame->fmt.media_type = MainMediaType::kMainTypeAudio;
        frame->fmt.sub_fmt.audio_fmt.type = SubMediaType::kSubTypePcm;
        frame->fmt.sub_fmt.audio_fmt.nbytes_per_sample = bytes_per_sample;
        frame->fmt.sub_fmt.audio_fmt.samples_per_channel = num_samples;
        frame->fmt.sub_fmt.audio_fmt.channels = num_channels;
        frame->fmt.sub_fmt.audio_fmt.samples_per_sec = samples_per_sec;
        frame->data_len[0] = len;
        frame->data[0] = new char[len];
        memcpy(frame->data[0], audio_samples, len);

        // 计算时间戳，根据采样频率进行单调递增
        timestamp_ += num_samples;
        frame->ts = timestamp_;

        if (KRTCGlobal::Instance()->engine_observer()) {
            KRTCGlobal::Instance()->engine_observer()->OnPureAudioFrame(frame);
        }
    }

    void ADMDataObserver::OnRenderData(const void* audio_samples,
        const size_t num_samples,
        const size_t bytes_per_sample,
        const size_t num_channels,
        const uint32_t samples_per_sec) {
       /* RTC_LOG(LS_INFO) << "音频播放数据，num_samples:" << num_samples
            << " bytes_per_sample:" << bytes_per_sample
            << " num_channels:" << num_channels
            << "samples_per_sec:" << samples_per_sec;*/
    }

}