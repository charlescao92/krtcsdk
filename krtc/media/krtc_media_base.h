#ifndef KRTCSDK_KRTC_MEDIA_KRTC_MEDIA_BASE_H_
#define KRTCSDK_KRTC_MEDIA_KRTC_MEDIA_BASE_H_

#include <string>

#include <api/peer_connection_interface.h>

#include "krtc/krtc.h"
#include "krtc/tools/utils.h"

namespace krtc {

class KRTCMediaBase {
public:
	explicit KRTCMediaBase(const CONTROL_TYPE& type, 
		const std::string& server_addr, 
		const std::string& channel = "", 
		const int& hwnd = 0);  // 拉流相关，内部渲染则传入窗口句柄，也可以自己实现获取裸流来渲染显示
	virtual ~KRTCMediaBase();

	virtual void Start() = 0;
	virtual void Stop() = 0;

protected:
	rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory_;
	rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;

	std::string httpRequestUrl_;
	std::string webrtcStreamUrl_;
	std::string channel_ = "livestream";
	int hwnd_ = 0;
};

} // namespace krtc

#endif // KRTCSDK_KRTC_MEDIA_KRTC_MEDIA_BASE_H_
