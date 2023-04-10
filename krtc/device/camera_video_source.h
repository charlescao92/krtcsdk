#ifndef XRTCSDK_XRTC_DEVICE_CAM_IMPL_H_
#define XRTCSDK_XRTC_DEVICE_CAM_IMPL_H_

#include "krtc/krtc.h"

namespace krtc {

	class CameraVideoSource : public IVideoHandler
	{
	public:
		void Start() override;
		void Stop() override;
		void Destroy() override;
	
	private:
		explicit CameraVideoSource(const char* cam_id);
		~CameraVideoSource();

		friend class KRTCEngine;
	};

} // namespace krtc

#endif // XRTCSDK_XRTC_DEVICE_CAM_IMPL_H_
