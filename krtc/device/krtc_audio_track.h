#ifndef KRTC_DEVICE_AUDIO_CAPTURER_H_
#define KRTC_DEVICE_AUDIO_CAPTURER_H_

#include <api/media_stream_interface.h>
#include <api/audio_options.h>
#include <api/notifier.h>
#include <api/scoped_refptr.h>

namespace krtc {

class LocalAudioSource : public webrtc::Notifier<webrtc::AudioSourceInterface> {
public:
    // Creates an instance of LocalAudioSource.
    static rtc::scoped_refptr<LocalAudioSource> Create(
        const cricket::AudioOptions* audio_options);

    webrtc::MediaSourceInterface::SourceState state() const override { 
        return webrtc::MediaSourceInterface::kLive; 
    }
    bool remote() const override { return false; }

    const cricket::AudioOptions options() const override { return options_; }

    void AddSink(webrtc::AudioTrackSinkInterface* sink) override {}
    void RemoveSink(webrtc::AudioTrackSinkInterface* sink) override {}

protected:
    LocalAudioSource() {}
    ~LocalAudioSource() override {}

private:
    void Initialize(const cricket::AudioOptions* audio_options);

    cricket::AudioOptions options_;
};

}

#endif // KRTC_DEVICE_AUDIO_CAPTURER_H_
