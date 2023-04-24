#ifndef KRTCSDK_KRTC_MEDIA_KRTC_PULL_IMPL_H_
#define KRTCSDK_KRTC_MEDIA_KRTC_PULL_IMPL_H_

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <api/media_stream_interface.h>
#include <api/peer_connection_interface.h>

#include "krtc/tools/utils.h"
#include "krtc/render/video_renderer.h"
#include "krtc/media/krtc_media_base.h"
#include "krtc/base/krtc_http.h"

namespace krtc {

class CRtcStatsCollector1 : public webrtc::RTCStatsCollectorCallback {
public:
    virtual void OnStatsDelivered(
          const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) override;

};

class KRTCPullImpl : public KRTCMediaBase, 
                     public webrtc::PeerConnectionObserver,
                     public webrtc::CreateSessionDescriptionObserver {
public:
    explicit KRTCPullImpl(const std::string& server_addr,
                          const std::string& pull_channel,
                          const int& hwnd);
    ~KRTCPullImpl();

    void Start();
    void Stop();

private:
    void GetRtcStats();

    // PeerConnectionObserver implementation.
    void OnSignalingChange(
        webrtc::PeerConnectionInterface::SignalingState new_state) override;

    void OnAddTrack(
        rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
        const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
        streams) override;

    void OnRemoveTrack(
        rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;

    void OnDataChannel(
        rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override;

    void OnIceGatheringChange(
        webrtc::PeerConnectionInterface::IceGatheringState new_state) override;

    void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override {}

    // CreateSessionDescriptionObserver implementation.
    void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;

    void OnFailure(webrtc::RTCError error) override;

    void handleHttpPullResponse(const HttpReply& reply);

private:
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>peer_connection_factory_;
    std::unique_ptr<VideoRenderer> remote_renderer_;

};

} // namespace krtc

#endif // KRTCSDK_KRTC_MEDIA_KRTC_PULL_IMPL_H_
