#include "krtc/device/camera_video_source.h"

#include "krtc/base/krtc_global.h"

namespace krtc {

CameraVideoSource::CameraVideoSource(const char* cam_id)
{
    KRTCGlobal::Instance()->CreateVcmCapturerSource(cam_id);
}

CameraVideoSource::~CameraVideoSource() {
    Destroy();
}

void CameraVideoSource::Start() {
    KRTCGlobal::Instance()->StartVcmCapturerSource();      
}

void CameraVideoSource::Stop() {
    KRTCGlobal::Instance()->StopVcmCapturerSource();
}

void CameraVideoSource::Destroy() {
}

} // namespace krtc
