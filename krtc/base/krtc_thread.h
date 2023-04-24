﻿#ifndef KRTCSDK_KRTC_MEDIA_BASE_KRTC_THREAD_H_
#define KRTCSDK_KRTC_MEDIA_BASE_KRTC_THREAD_H_

#include <rtc_base/thread.h>

namespace krtc {

class KRTCThread {
public:
    KRTCThread(rtc::Thread* current_thread);
    rtc::Thread* CurrentThread() { return current_thread_; }

private:
    rtc::Thread* current_thread_;

};

} // namespace krtc

#endif // KRTCSDK_KRTC_MEDIA_BASE_KRTC_THREAD_H_