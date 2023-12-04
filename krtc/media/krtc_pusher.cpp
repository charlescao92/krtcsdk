#include <rtc_base/thread.h>
#include <rtc_base/logging.h>
#include <rtc_base/string_encode.h>

#include "krtc/media/krtc_pusher.h"
#include "krtc/base/krtc_global.h"
#include "krtc/base/krtc_thread.h"
#include "krtc/media/krtc_push_impl.h"

namespace krtc {

KRTCPusher::KRTCPusher(const std::string& server_addr, const std::string& push_channel) :
    current_thread_(std::make_unique<KRTCThread>(rtc::Thread::Current()))
{
    push_impl_ = new rtc::RefCountedObject<KRTCPushImpl>(server_addr, push_channel);  
}

KRTCPusher::~KRTCPusher() = default;

void KRTCPusher::Start() {
    RTC_LOG(LS_INFO) << "KRTCPusher Start";

    if (push_impl_) {
        push_impl_->Start();
    }
}

void KRTCPusher::Stop() {
    RTC_LOG(LS_INFO) << "KRTCPusher Stop";

    if (push_impl_) {
        push_impl_->Stop();
        push_impl_ = nullptr;
    }
}

void KRTCPusher::Destroy() {
    RTC_LOG(LS_INFO) << "KRTCPusher Destroy";

    delete this;
}

void KRTCPusher::SetEnableVideo(bool enable)
{
    if (push_impl_) {
        push_impl_->SetEnableVideo(enable);
    }
}

void KRTCPusher::SetEnableAudio(bool enable)
{
    if (push_impl_) {
        push_impl_->SetEnableAudio(enable);
    }
}

} // namespace krtc
