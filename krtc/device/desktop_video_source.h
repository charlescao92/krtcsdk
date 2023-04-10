#ifndef XRTCSDK_XRTC_DEVICE_DESKTOP_VIDEO_SOURCE_H_
#define XRTCSDK_XRTC_DEVICE_DESKTOP_VIDEO_SOURCE_H_

#include "krtc/krtc.h"

namespace krtc {

	class DesktopVideoSource : public IVideoHandler
	{
	public:
		void Start() override;
		void Stop() override;
		void Destroy() override;

	private:
		DesktopVideoSource(uint16_t screen_index = 0, uint16_t target_fps = 30);
		~DesktopVideoSource();

		friend class KRTCEngine;
	};

} // namespace krtc

#endif // XRTCSDK_XRTC_DEVICE_DESKTOP_VIDEO_SOURCE_H_
