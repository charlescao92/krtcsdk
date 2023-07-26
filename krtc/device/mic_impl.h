#ifndef KRTCSDK_KRTC_DEVICE_MIC_IMPL_H_
#define KRTCSDK_KRTC_DEVICE_MIC_IMPL_H_

#include <mutex>

#include <rtc_base/thread.h>
#include <modules/audio_device/include/audio_device.h>

#include "krtc/krtc.h"

namespace krtc {

class MicImpl : public IAudioHandler
{
private:
    void Start() override;
    void Stop() override;
    void Destroy() override;
    void SetEnableVideo(bool enable) {}
    void SetEnableAudio(bool enable) {}

private:
    explicit MicImpl(const char* mic_id);
    ~MicImpl() override;

private:
    std::string mic_id_;
    rtc::Thread* signaling_thread_;
    bool has_start_ = false;
    std::mutex mtx_;

    friend class KRTCEngine;
};

} // namespace krtc

#endif // KRTCSDK_KRTC_DEVICE_MIC_IMPL_H_
