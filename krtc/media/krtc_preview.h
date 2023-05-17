#ifndef KRTCSDK_KRTC_MEDIA_KRTC_PREVIEW_H_
#define KRTCSDK_KRTC_MEDIA_KRTC_PREVIEW_H_

#include "krtc/krtc.h"

#include <api/media_stream_interface.h>

namespace krtc {

class KRTCThread;
class VideoRenderer;

class KRTCPreview : public IMediaHandler,
					public rtc::VideoSinkInterface<webrtc::VideoFrame> {
public:
	explicit KRTCPreview(const int& hwnd = 0);
	~KRTCPreview();

public:
	void Start();
	void Stop();
	void Destroy();
	void SetEnableVideo(bool enable) {}
	void SetEnableAudio(bool enable) {}

private:
	void OnFrame(const webrtc::VideoFrame& frame) override;

private:
	std::unique_ptr<KRTCThread> current_thread_;
	std::unique_ptr<VideoRenderer> local_renderer_;
	int hwnd_;
};

}
#endif // KRTCSDK_KRTC_MEDIA_KRTC_PREVIEW_H_