#include "krtc/media/krtc_media_base.h"

#include <rtc_base/string_encode.h>

namespace krtc {

KRTCMediaBase::KRTCMediaBase(const CONTROL_TYPE& type, 
	const std::string& server_addr, 
	const std::string& channel, 
	const int& hwnd) :
	hwnd_(hwnd)
{
	// server_addr  = https://charlescao92.cn:1986

	std::vector<std::string> splits;
	if (!rtc::tokenize(server_addr, '/', &splits)) {
		return;
	}

	if (splits.size() < 2) {
		return;
	}

	std::string server_ip, server_port;
	if (!rtc::tokenize_first(splits[1], ':', &server_ip, &server_port)) {
		return;
	}

	std::string control;
	if (type == CONTROL_TYPE::PUSH) {
		// http://1.14.148.67:1985/rtc/v1/publish/
		control = "publish";
	}
	else if (type == CONTROL_TYPE::PULL) {
		// http://1.14.148.67:1985/rtc/v1/play/
		control = "play";
	}

	httpRequestUrl_ = server_addr + "/rtc/v1/" + control + "/";

	if (!channel.empty()) {
		channel_ = channel;
	}

	// webrtc://1.14.148.67/live/livestream
	webrtcStreamUrl_ = "webrtc://" + server_ip + "/live/" + channel_;
}

} // namespace krtc
