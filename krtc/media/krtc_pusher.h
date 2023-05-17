#ifndef KRTCSDK_KRTC_MEDIA_KRTC_PUSHER_H_
#define KRTCSDK_KRTC_MEDIA_KRTC_PUSHER_H_

#include "krtc/krtc.h"

namespace krtc {

class KRTCThread;
class KRTCPushImpl;

class KRTCPusher : public IMediaHandler {
private:
    void Start();
    void Stop();
    void Destroy();

    void SetEnableVideo(bool enable = true);
    void SetEnableAudio(bool enable = true);

private:
    explicit KRTCPusher(const std::string& server_addr, const std::string& push_channel = "livestream");
    ~KRTCPusher();

private:
    std::unique_ptr<KRTCThread> current_thread_;
    rtc::scoped_refptr<KRTCPushImpl> push_impl_;

    friend class KRTCEngine;
};

} // namespace krtc

#endif // KRTCSDK_KRTC_MEDIA_BASE_KRTC_PUSHER_H_
