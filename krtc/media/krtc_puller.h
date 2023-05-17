#ifndef KRTCSDK_KRTC_MEDIA_KRTC_PULLER_H_
#define KRTCSDK_KRTC_MEDIA_KRTC_PULLER_H_

#include "krtc/krtc.h"

namespace krtc {

class KRTCThread;
class KRTCPullImpl;

class KRTCPuller : public IMediaHandler {
private:
    void Start();
    void Stop();
    void Destroy();

    void SetEnableVideo(bool enable) {}
    void SetEnableAudio(bool enable) {}

private:
    explicit KRTCPuller(const std::string& server_addr, const std::string& push_channel = "livestream", int hwnd = 0);
    ~KRTCPuller();

    friend class KRTCEngine;

private:
    std::unique_ptr<KRTCThread> current_thread_;
    rtc::scoped_refptr<KRTCPullImpl> pull_impl_;
};

} // namespace krtc

#endif // KRTCSDK_KRTC_MEDIA_KRTC_PULLER_H_