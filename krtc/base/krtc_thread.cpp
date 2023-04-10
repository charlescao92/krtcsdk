#include "krtc/base/krtc_thread.h"

namespace krtc {
	KRTCThread::KRTCThread(rtc::Thread* current_thread) :
		current_thread_(current_thread) {}

}