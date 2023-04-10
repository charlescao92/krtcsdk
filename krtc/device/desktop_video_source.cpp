#include "krtc/device/desktop_video_source.h"
#include "krtc/base/krtc_global.h"

namespace krtc {

DesktopVideoSource::DesktopVideoSource(uint16_t screen_index, uint16_t target_fps) {
	KRTCGlobal::Instance()->CreateDesktopCapturerSource(screen_index, target_fps);
}

DesktopVideoSource::~DesktopVideoSource() {}

void DesktopVideoSource::Start() {
	KRTCGlobal::Instance()->StartDesktopCapturerSource();
}

void DesktopVideoSource::Stop() {
	KRTCGlobal::Instance()->StopDesktopCapturerSource();
}

void DesktopVideoSource::Destroy() {}

} // namespace krtc
