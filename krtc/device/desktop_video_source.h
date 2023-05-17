#ifndef KRTCSDK_KRTC_DEVICE_DESKTOP_VIDEO_SOURCE_H_
#define KRTCSDK_KRTC_DEVICE_DESKTOP_VIDEO_SOURCE_H_

#include "krtc/krtc.h"

namespace krtc {

class DesktopVideoSource : public IVideoHandler
{
private:
	void Start() override;
	void Stop() override;
	void Destroy() override;
	void SetEnableVideo(bool enable) {}
	void SetEnableAudio(bool enable) {}

private:
	DesktopVideoSource(uint16_t screen_index = 0, uint16_t target_fps = 30);
	~DesktopVideoSource();

	friend class KRTCEngine;
};

} // namespace krtc

#endif // KRTCSDK_KRTC_DEVICE_DESKTOP_VIDEO_SOURCE_H_
