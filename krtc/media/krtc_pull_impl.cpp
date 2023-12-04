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

namespace krtc {

void CRtcStatsCollector1::OnStatsDelivered(
    const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) {
    Json::Reader reader;

    for (auto it = report->begin(); it != report->end(); ++it) {
        // "type" : "inbound-rtp"
        Json::Value jmessage;
        if (!reader.parse(it->ToJson(), jmessage)) {
            RTC_LOG(LS_WARNING) << "stats report invalid!!!";
            return;
        }

        std::string type = jmessage["type"].asString();
        if (type == "inbound-rtp") {

            RTC_LOG(LS_INFO) << "Stats report : " << it->ToJson();
        }
    }
}

KRTCPullImpl::KRTCPullImpl(
    const std::string& server_addr,
    const std::string& pull_channel,
    const int& hwnd) :
    KRTCMediaBase(CONTROL_TYPE::PULL, server_addr, pull_channel, hwnd)
{
    KRTCGlobal::Instance()->http_manager()->AddObject(this);
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

    webrtc::PeerConnectionFactoryInterface::Options options;
    options.disable_encryption = false;
    peer_connection_factory_->SetOptions(options);

    peer_connection_ = peer_connection_factory_->CreatePeerConnection(
        config, nullptr, nullptr, this);

    webrtc::RtpTransceiverInit rtpTransceiverInit;
    rtpTransceiverInit.direction = webrtc::RtpTransceiverDirection::kRecvOnly;
    peer_connection_->AddTransceiver(cricket::MediaType::MEDIA_TYPE_AUDIO, rtpTransceiverInit);
    peer_connection_->AddTransceiver(cricket::MediaType::MEDIA_TYPE_VIDEO, rtpTransceiverInit);

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
    peer_connection_->GetStats(stats.get());
}

// PeerConnectionObserver implementation.
void KRTCPullImpl::OnSignalingChange(
    webrtc::PeerConnectionInterface::SignalingState new_state) {
}

void KRTCPullImpl::OnAddTrack(
    rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
    const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&streams)
{
    auto* track = reinterpret_cast<webrtc::MediaStreamTrackInterface*>(
        receiver->track().release());

    if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
        auto* video_track = static_cast<webrtc::VideoTrackInterface*>(track);

        remote_renderer_ = VideoRenderer::Create(CONTROL_TYPE::PULL, hwnd_, 1, 1);
        video_track->AddOrUpdateSink(remote_renderer_.get(), rtc::VideoSinkWants());
    }

    track->Release();
}

void KRTCPullImpl::OnRemoveTrack(
    rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) {
}

void KRTCPullImpl::OnDataChannel(
    rtc::scoped_refptr<webrtc::DataChannelInterface> channel) {
}

void KRTCPullImpl::OnIceGatheringChange(
    webrtc::PeerConnectionInterface::IceGatheringState new_state) {
}

// CreateSessionDescriptionObserver implementation.
void KRTCPullImpl::OnSuccess(webrtc::SessionDescriptionInterface* desc) {

    peer_connection_->SetLocalDescription(
        DummySetSessionDescriptionObserver::Create().get(), desc);

    std::string sdpOffer;
    desc->ToString(&sdpOffer);
    RTC_LOG(LS_INFO) << "sdp Offer:" << sdpOffer;

    Json::Value reqMsg;
    reqMsg["api"] = httpRequestUrl_;
    reqMsg["streamurl"] = webrtcStreamUrl_;
    reqMsg["sdp"] = sdpOffer;
    reqMsg["tid"] = rtc::CreateRandomString(7);
    Json::StreamWriterBuilder write_builder;
    write_builder.settings_["indentation"] = "";
    std::string json_data = Json::writeString(write_builder, reqMsg);

    RTC_LOG(LS_INFO) << "send webrtc pull request.....";

    HttpRequest request(httpRequestUrl_, json_data);
    KRTCGlobal::Instance()->http_manager()->Post(request, [=](HttpReply reply) {

        KRTCGlobal::Instance()->api_thread()->PostTask([=]() {
            handleHttpPullResponse(reply);
        });

    }, this);
}

void KRTCPullImpl::OnFailure(webrtc::RTCError error) {
    if (KRTCGlobal::Instance()->engine_observer()) {
        KRTCGlobal::Instance()->engine_observer()->OnPullFailed(KRTCError::kCreateOfferErr);
    }
}

void KRTCPullImpl::handleHttpPullResponse(const HttpReply &reply)
{
    if (reply.get_status_code() != 200 || reply.get_errno() != 0) {
        RTC_LOG(LS_INFO) << "http post error";

        if (KRTCGlobal::Instance()->engine_observer()) {
            KRTCGlobal::Instance()->engine_observer()->OnPullFailed(KRTCError::kSendOfferErr);
        }
        return;
    }

    std::string responseBody = reply.get_resp();
    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    Json::Value root;
    JSONCPP_STRING err;
    reader->parse(responseBody.data(), responseBody.data() + responseBody.size(), &root, &err);
    if (!err.empty()) {
        RTC_LOG(LS_WARNING) << "Received unknown message. " << responseBody;

        if (KRTCGlobal::Instance()->engine_observer()) {
            KRTCGlobal::Instance()->engine_observer()->OnPullFailed(KRTCError::kParseAnswerErr);
        }
        return;
    }

    RTC_LOG(LS_INFO) << "http pull response : " << responseBody;

    int code = root["code"].asInt();
    if (code != 0) {
        RTC_LOG(LS_INFO) << "http pull response error, code: " << code;

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

    peer_connection_->SetRemoteDescription(
        DummySetSessionDescriptionObserver::Create().get(),
        session_description.release());

    if (KRTCGlobal::Instance()->engine_observer()) {
        KRTCGlobal::Instance()->engine_observer()->OnPullSuccess();
    }
}

}  // namespace krtc
