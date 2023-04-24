#include <rtc_base/thread.h>
#include <rtc_base/logging.h>
#include <rtc_base/task_utils/to_queued_task.h>
#include <rtc_base/string_encode.h>

#include "krtc/media/krtc_puller.h"
#include "krtc/base/krtc_global.h"
#include "krtc/base/krtc_thread.h"
#include "krtc/media/krtc_pull_impl.h"

namespace krtc {

KRTCPuller::KRTCPuller(const std::string& server_addr, const std::string& push_channel, int hwnd) 
    :current_thread_(std::make_unique<KRTCThread>(rtc::Thread::Current()))
{
    pull_impl_ = new rtc::RefCountedObject<KRTCPullImpl>(server_addr, push_channel, hwnd);
}

KRTCPuller::~KRTCPuller() {
}

void KRTCPuller::Start() {
    RTC_LOG(LS_INFO) << "KRTCPuller Start";

    if (pull_impl_) {
        pull_impl_->Start();
    }
}

void KRTCPuller::Stop() {
    RTC_LOG(LS_INFO) << "KRTCPuller Stop";

    if (pull_impl_) {
        pull_impl_->Stop();
        pull_impl_ = nullptr;
    }
}

void KRTCPuller::Destroy() {
    RTC_LOG(LS_INFO) << "KRTCPuller Destroy";

    delete this;
}

} // namespace krtc
