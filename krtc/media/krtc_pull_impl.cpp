#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>
#include <vector>
#include <thread>


#include <absl/memory/memory.h>
#include <absl/types/optional.h>
#include <api/audio/audio_mixer.h>
#include <api/audio_codecs/audio_decoder_factory.h>
#include <api/audio_codecs/audio_encoder_factory.h>
#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/audio_options.h>
#include <api/create_peerconnection_factory.h>
#include <api/rtp_sender_interface.h>
#include <api/video_codecs/builtin_video_decoder_factory.h>
#include <api/video_codecs/builtin_video_encoder_factory.h>
#include <api/video_codecs/video_decoder_factory.h>
#include <api/video_codecs/video_encoder_factory.h>
#include <modules/audio_device/include/audio_device.h>
#include <modules/audio_processing/include/audio_processing.h>
#include <modules/video_capture/video_capture.h>
#include <modules/video_capture/video_capture_factory.h>
#include <pc/video_track_source.h>
#include <rtc_base/checks.h>
#include <rtc_base/logging.h>
#include <rtc_base/ref_counted_object.h>
#include <pc/rtc_stats_collector.h>
#include <rtc_base/strings/json.h>

#include "krtc/media/default.h"
#include "krtc/media/krtc_pull_impl.h"
#include "krtc/base/krtc_global.h"
#include "krtc/base/krtc_http.h"

namespace krtc {

    void CRtcStatsCollector1::OnStatsDelivered(
        const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) {
        Json::Reader reader;

        for (auto it = report->begin(); it != report->end(); ++it) {
            // "type" : "inbound-rtp"
            Json::Value jmessage;
            if (!reader.parse(it->ToJson(), jmessage)) {
                RTC_LOG(WARNING) << "stats report invalid!!!";
                return;
            }

            std::string type = jmessage["type"].asString();
            if (type == "inbound-rtp") {

                RTC_LOG(INFO) << "Stats report : " << it->ToJson();
            }
        }
    }

    KRTCPullImpl::KRTCPullImpl(
        const std::string& server_addr, 
        const std::string& pull_channel, 
        const int& hwnd) :
        KRTCMediaBase(STREAM_CONTROL_TYPE::PULL_TYPE, server_addr, pull_channel, hwnd)
    {
       
    }

    KRTCPullImpl::~KRTCPullImpl() {
        RTC_DCHECK(!peer_connection_);
    }

    void KRTCPullImpl::Start() {
        RTC_LOG(LS_INFO) << "KRTCPullImpl Start";

        peer_connection_factory_ = webrtc::CreatePeerConnectionFactory(
            KRTCGlobal::Instance()->network_thread() /* network_thread */, 
            KRTCGlobal::Instance()->worker_thread() /* worker_thread */,
            KRTCGlobal::Instance()->api_thread() /* signaling_thread */, 
            nullptr /* default_adm */,
            webrtc::CreateBuiltinAudioEncoderFactory(),
            webrtc::CreateBuiltinAudioDecoderFactory(),
            webrtc::CreateBuiltinVideoEncoderFactory(),
            webrtc::CreateBuiltinVideoDecoderFactory(), nullptr /* audio_mixer */,
            nullptr /* audio_processing */);

        webrtc::PeerConnectionInterface::RTCConfiguration config;
        config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
        config.enable_dtls_srtp = true;

        peer_connection_ = peer_connection_factory_->CreatePeerConnection(
            config, nullptr, nullptr, this);

        webrtc::RtpTransceiverInit rtpTransceiverInit;
        rtpTransceiverInit.direction = webrtc::RtpTransceiverDirection::kRecvOnly;
        peer_connection_->AddTransceiver(cricket::MediaType::MEDIA_TYPE_AUDIO,
            rtpTransceiverInit);
        peer_connection_->AddTransceiver(cricket::MediaType::MEDIA_TYPE_VIDEO, rtpTransceiverInit);

        //创建Offer SDP成功的话，会回调OnSuccess
        peer_connection_->CreateOffer(this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
    }

    void KRTCPullImpl::Stop() {
        RTC_LOG(LS_INFO) << "KRTCPullImpl Stop";

        peer_connection_ = nullptr;
        peer_connection_factory_ = nullptr;
        remote_renderer_ = nullptr;
    }

    void KRTCPullImpl::GetRtcStats() {
        rtc::scoped_refptr<CRtcStatsCollector1> stats(
            new rtc::RefCountedObject<CRtcStatsCollector1>());
        peer_connection_->GetStats(stats);
    }

    // PeerConnectionObserver implementation.
    void KRTCPullImpl::OnSignalingChange(
        webrtc::PeerConnectionInterface::SignalingState new_state) {
        RTC_LOG(INFO) << __FUNCTION__;
    }

    void KRTCPullImpl::OnAddTrack(
        rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
        const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&streams) 
    {
        RTC_LOG(INFO) << __FUNCTION__;

        auto* track = reinterpret_cast<webrtc::MediaStreamTrackInterface*>(
            receiver->track().release());

        if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
            auto* video_track = static_cast<webrtc::VideoTrackInterface*>(track);
            //渲染远端视频
            remote_renderer_ = VideoRenderer::Create(hwnd_, 1, 1);
            video_track->AddOrUpdateSink(remote_renderer_.get(), rtc::VideoSinkWants());
        }

        track->Release();
    }

    void KRTCPullImpl::OnRemoveTrack(
        rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) {
        RTC_LOG(INFO) << __FUNCTION__;
    }

    void KRTCPullImpl::OnDataChannel(
        rtc::scoped_refptr<webrtc::DataChannelInterface> channel) {
        RTC_LOG(INFO) << __FUNCTION__;
    }

    void KRTCPullImpl::OnIceGatheringChange(
        webrtc::PeerConnectionInterface::IceGatheringState new_state) {
        RTC_LOG(INFO) << __FUNCTION__;
    }

    // CreateSessionDescriptionObserver implementation.
    void KRTCPullImpl::OnSuccess(webrtc::SessionDescriptionInterface* desc) {
        RTC_LOG(INFO) << __FUNCTION__;

        peer_connection_->SetLocalDescription(
            DummySetSessionDescriptionObserver::Create(), desc);

        // SRS4.0 webrtc当前只支持h.264
        std::string sdpOffer;
        desc->ToString(&sdpOffer);
        RTC_LOG(INFO) << "sdp Offer:" << sdpOffer;

        Json::Value reqMsg;
        reqMsg["api"] = httpRequestUrl_;
        reqMsg["streamurl"] = webrtcStreamUrl_;
        reqMsg["sdp"] = sdpOffer;
        Json::StreamWriterBuilder write_builder;
        write_builder.settings_["indentation"] = "";
        std::string json_data = Json::writeString(write_builder, reqMsg);

        RTC_LOG(LS_INFO) << "send webrtc pull request.....";

        std::string responseBody;
        Http::HTTPERROR errCode = httpExecRequest("POST", httpRequestUrl_, responseBody, json_data);
        if (errCode != Http::HTTPERROR_SUCCESS) {
            RTC_LOG(INFO) << "http post error : " << httpErrorString(errCode);

            if (KRTCGlobal::Instance()->engine_observer()) {
                KRTCGlobal::Instance()->engine_observer()->OnPullFailed(KRTCError::kSendOfferErr);
            }
            return;
        }

        // 解析srs服务器发回来的json数据
        Json::Reader reader;
        Json::Value root;
        if (!reader.parse(responseBody, root)) {
            RTC_LOG(WARNING) << "Received unknown message. " << responseBody;

            if (KRTCGlobal::Instance()->engine_observer()) {
                KRTCGlobal::Instance()->engine_observer()->OnPullFailed(KRTCError::kParseAnswerErr);
            }
            return;
        }

        RTC_LOG(INFO) << "http pull response : " << responseBody;

        int code = root["code"].asInt();
        if (code != 0) {
            RTC_LOG(INFO) << "http pull response error, code: " << code;

            if (KRTCGlobal::Instance()->engine_observer()) {
                KRTCGlobal::Instance()->engine_observer()->OnPullFailed(KRTCError::kAnswerResponseErr);
            }
            return;
        }

        std::string sdpAnswer = root["sdp"].asString();

        webrtc::SdpParseError error;
        webrtc::SdpType type = webrtc::SdpType::kAnswer;
        std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
            webrtc::CreateSessionDescription(type, sdpAnswer, &error);

        //设置Answer SDP成功的话，会回调OnAddTrack
        peer_connection_->SetRemoteDescription(
            DummySetSessionDescriptionObserver::Create(),
            session_description.release());

        if (KRTCGlobal::Instance()->engine_observer()) {
            KRTCGlobal::Instance()->engine_observer()->OnPullSuccess();
        }
    }

    void KRTCPullImpl::OnFailure(webrtc::RTCError error) {
        RTC_LOG(INFO) << __FUNCTION__;

        if (KRTCGlobal::Instance()->engine_observer()) {
            KRTCGlobal::Instance()->engine_observer()->OnPullFailed(KRTCError::kCreateOfferErr);
        }
    }

}  // namespace krtc
