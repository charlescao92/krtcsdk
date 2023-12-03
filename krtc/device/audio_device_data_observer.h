#ifndef KRTCSDK_KRTC_DEVICE_AUDIO_DEVICE_DATA_OBSERVER_H_
#define KRTCSDK_KRTC_DEVICE_AUDIO_DEVICE_DATA_OBSERVER_H_

#include <modules/audio_device/include/audio_device_data_observer.h>
#include <rtc_base/logging.h>

namespace krtc{

class ADMDataObserver : public webrtc::AudioDeviceDataObserver {
private:
    virtual void OnCaptureData(const void* audio_samples,
        const size_t num_samples,
        const size_t bytes_per_sample,
        const size_t num_channels,
        const uint32_t samples_per_sec) override;
     
    virtual void OnRenderData(const void* audio_samples,
        const size_t num_samples,
        const size_t bytes_per_sample,
        const size_t num_channels,
        const uint32_t samples_per_sec) override;

private:
    uint32_t timestamp_ = 0;

};


} // end namespace krtc

#endif // KRTCSDK_KRTC_DEVICE_AUDIO_DEVICE_DATA_OBSERVER_H_