#ifndef KRTCSDK_KRTC_MEDIA_BASE_DEFAULT_H_
#define KRTCSDK_KRTC_MEDIA_BASE_DEFAULT_H_

#include <rtc_base/ref_counted_object.h>
#include <api/jsep.h>
#include <rtc_base/logging.h>
#include <api/make_ref_counted.h>

const char kAudioLabel[] = "audio_label";
const char kVideoLabel[] = "video_label";
const char kStreamId[] = "stream_id";

class DummySetSessionDescriptionObserver
    : public webrtc::SetSessionDescriptionObserver {
public:
    static rtc::scoped_refptr<DummySetSessionDescriptionObserver> Create() {
        return rtc::make_ref_counted<DummySetSessionDescriptionObserver>();
    }
    virtual void OnSuccess() { RTC_LOG(LS_INFO) << __FUNCTION__; }
    virtual void OnFailure(webrtc::RTCError error) {
        RTC_LOG(LS_INFO) << __FUNCTION__ << " " << ToString(error.type()) << ": "
            << error.message();
    }
};

#endif // KRTCSDK_KRTC_MEDIA_BASE_DEFAULT_H_