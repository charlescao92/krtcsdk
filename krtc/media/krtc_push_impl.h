
#ifndef KRTCSDK_KRTC_MEDIA_KRTC_PUSH_IMPL_H_
#define KRTCSDK_KRTC_MEDIA_KRTC_PUSH_IMPL_H_

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <api/media_stream_interface.h>

#include "krtc/media/krtc_media_base.h"
#include "krtc/media/stats_collector.h"
#include "krtc/base/krtc_http.h"

class CTimer;

namespace krtc {

class KRTCPushImpl : public KRTCMediaBase,
                     public webrtc::PeerConnectionObserver,
                     public webrtc::CreateSessionDescriptionObserver,
                     public StatsObserver
{
public:
    explicit KRTCPushImpl(const std::string& server_addr, const std::string& push_channel = "");
    ~KRTCPushImpl();

    void Start();
    void Stop();
    void GetRtcStats();

    void SetEnableVideo(bool enable = true);
    void SetEnableAudio(bool enable = true);

private:
    // PeerConnectionObserver implementation.
    void OnSignalingChange(
        webrtc::PeerConnectionInterface::SignalingState new_state) override {}

    void OnDataChannel(
        rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override;

    void OnIceGatheringChange(
        webrtc::PeerConnectionInterface::IceGatheringState new_state) override {}

    void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override {}

    // CreateSessionDescriptionObserver implementation.
    void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;

    void OnFailure(webrtc::RTCError error) override;

    // StatsObserver implementation.
    void OnStatsInfo(const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report);

    void handleHttpPushResponse(const HttpReply& reply);

private:
    rtc::scoped_refptr<CRtcStatsCollector> stats_;
    std::unique_ptr<CTimer> stats_timer_;

    rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track_;
    rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track_;
};

} // namespace krtc

#endif  // KRTCSDK_KRTC_MEDIA_KRTC_PUSH_IMPL_H_
