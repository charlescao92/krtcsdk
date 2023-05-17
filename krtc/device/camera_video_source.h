#ifndef KRTCSDK_KRTC_DEVICE_CAMERA_VIDEO_SOURCE_H_
#define KRTCSDK_KRTC_DEVICE_CAMERA_VIDEO_SOURCE_H_

#include "krtc/krtc.h"

namespace krtc {

class CameraVideoSource : public IVideoHandler
{
public:
	void Start() override;
	void Stop() override;
	void Destroy() override;
	void SetEnableVideo(bool enable) {}
	void SetEnableAudio(bool enable) {}

private:
	explicit CameraVideoSource(const char* cam_id);
	~CameraVideoSource();

	friend class KRTCEngine;
};

} // namespace krtc

#endif // KRTCSDK_KRTC_DEVICE_CAMERA_VIDEO_SOURCE_H_
