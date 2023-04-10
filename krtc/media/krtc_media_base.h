#ifndef KRTCSDK_KRTC_MEDIA_KRTC_MEDIA_BASE_H_
#define KRTCSDK_KRTC_MEDIA_KRTC_MEDIA_BASE_H_

#include <string>

#include <api/peer_connection_interface.h>

#include "krtc/krtc.h"
#include "krtc/tools/utils.h"

namespace krtc {

enum class STREAM_CONTROL_TYPE {
	PUSH_TYPE,
	PULL_TYPE,
};

class KRTCMediaBase {
public:
	explicit KRTCMediaBase(const STREAM_CONTROL_TYPE& type, 
		const std::string& server_addr, 
		const std::string& channel = "", 
		const int& hwnd = 0);  // ������أ��ڲ���Ⱦ���봰�ھ����Ҳ�����Լ�ʵ�ֻ�ȡ��������Ⱦ��ʾ
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