#include "krtc/media/krtc_push_impl.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>
#include <vector>

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
#include <rtc_base/strings/json.h>

#include "krtc/media/default.h"
#include "krtc/device/vcm_capturer.h"
#include "krtc/base/krtc_http.h"
#include "krtc/base/krtc_global.h"
#include "krtc/device/audio_track.h"
#include "krtc/tools/timer.h"

namespace krtc {

KRTCPushImpl::KRTCPushImpl(const std::string& server_addr, const std::string& push_channel) :
    KRTCMediaBase(CONTROL_TYPE::PUSH, server_addr, push_channel)
{
    KRTCGlobal::Instance()->http_manager()->AddObject(this);
}

KRTCPushImpl::~KRTCPushImpl() {
    RTC_DCHECK(!peer_connection_);
}

void KRTCPushImpl::Start() {
    RTC_LOG(LS_INFO) << "KRTCPushImpl Start";

    webrtc::PeerConnectionInterface::RTCConfiguration config;
    config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
    config.enable_dtls_srtp = true;

    webrtc::PeerConnectionFactoryInterface* peer_connection_factory =
        KRTCGlobal::Instance()->push_peer_connection_factory();
    peer_connection_ = peer_connection_factory->CreatePeerConnection(
        config, nullptr, nullptr, this);

    webrtc::RtpTransceiverInit rtpTransceiverInit;
    rtpTransceiverInit.direction = webrtc::RtpTransceiverDirection::kSendOnly;
    peer_connection_->AddTransceiver(cricket::MediaType::MEDIA_TYPE_AUDIO,
        rtpTransceiverInit);
    peer_connection_->AddTransceiver(cricket::MediaType::MEDIA_TYPE_VIDEO,
        rtpTransceiverInit);

    cricket::AudioOptions options;
    rtc::scoped_refptr<LocalAudioSource> audio_source(LocalAudioSource::Create(&options));

    audio_track_ = peer_connection_factory->CreateAudioTrack(kAudioLabel, audio_source);
    auto add_audio_track_result = peer_connection_->AddTrack(audio_track_, { kStreamId });
    if (!add_audio_track_result.ok()) {
        RTC_LOG(LS_ERROR) << "Failed to add audio track to PeerConnection: "
            << add_audio_track_result.error().message();
    }

    video_track_ = peer_connection_factory->CreateVideoTrack(
        kVideoLabel, KRTCGlobal::Instance()->current_video_source());
    auto add_video_track_result = peer_connection_->AddTrack(video_track_, { kStreamId });
    if (!add_video_track_result.ok()) {
        RTC_LOG(LS_ERROR) << "Failed to add video track to PeerConnection: "
            << add_video_track_result.error().message();

    }

    if (!add_audio_track_result.ok() && !add_video_track_result.ok()) {
        if (KRTCGlobal::Instance()->engine_observer()) {
            KRTCGlobal::Instance()->engine_observer()->OnPushFailed(KRTCError::kAddTrackErr);
        }

        return;
    }

    peer_connection_->CreateOffer(
        this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());

}

void KRTCPushImpl::Stop()
{
    if (stats_timer_) {
        stats_timer_->Stop();
        stats_timer_ = nullptr;
    }

    if (peer_connection_) {
        peer_connection_ = nullptr;
    }
}

void KRTCPushImpl::GetRtcStats() {
    KRTCGlobal::Instance()->api_thread()->PostTask(webrtc::ToQueuedTask([=]() {
        RTC_LOG(LS_INFO) << "KRTCPushImpl  GetRtcStats";
        if (!stats_) {
            stats_ = new rtc::RefCountedObject<CRtcStatsCollector>(this);
        }
        if (peer_connection_) {
            peer_connection_->GetStats(stats_.get());
        }
     }));
}

void KRTCPushImpl::SetEnableVideo(bool enable)
{
    if (!peer_connection_) {
        return;
    }

    if (video_track_) {
        video_track_->set_enabled(enable);
    }
}

void KRTCPushImpl::SetEnableAudio(bool enable)
{
    if (!peer_connection_) {
        return;
    }

    if (audio_track_) {
        audio_track_->set_enabled(enable);
    }
}

void KRTCPushImpl::OnDataChannel(
    rtc::scoped_refptr<webrtc::DataChannelInterface> channel) {
}

// CreateSessionDescriptionObserver implementation.
void KRTCPushImpl::OnSuccess(webrtc::SessionDescriptionInterface* desc) {
    peer_connection_->SetLocalDescription(
        DummySetSessionDescriptionObserver::Create(), desc);

    std::string sdpOffer;
    desc->ToString(&sdpOffer);
    RTC_LOG(INFO) << "sdp offer:" << sdpOffer;

    Json::Value reqMsg;
    reqMsg["api"] = "http://1.14.148.67:1985/rtc/v1/publish?codec=hevc";// httpRequestUrl_;
    reqMsg["streamurl"] = webrtcStreamUrl_ + "?codec=hevc";
    reqMsg["sdp"] = sdpOffer;
    reqMsg["tid"] = rtc::CreateRandomString(7);
    Json::StreamWriterBuilder write_builder;
    write_builder.settings_["indentation"] = "";
    std::string json_data = Json::writeString(write_builder, reqMsg);

    RTC_LOG(LS_INFO) << "send webrtc push request.....";

    HttpRequest request(httpRequestUrl_, json_data);
    KRTCGlobal::Instance()->http_manager()->Post(request, [=](HttpReply reply) {
        RTC_LOG(LS_INFO) << "signaling push response, url: " << reply.get_url()
            << ", body: " << reply.get_body()
            << ", status: " << reply.get_status_code()
            << ", err_no: " << reply.get_errno()
            << ", err_msg: " << reply.get_err_msg()
            << ", response: " << reply.get_resp();
        KRTCGlobal::Instance()->api_thread()->PostTask(webrtc::ToQueuedTask([=]() {
            handleHttpPushResponse(reply);
        }));

    }, this);

}

void KRTCPushImpl::OnFailure(webrtc::RTCError error) {
    if (KRTCGlobal::Instance()->engine_observer()) {
        KRTCGlobal::Instance()->engine_observer()->OnPushFailed(KRTCError::kCreateOfferErr);
    }
}

void KRTCPushImpl::OnStatsInfo(const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) {
    Json::Reader reader;

    for (auto it = report->begin(); it != report->end(); ++it) {
        Json::Value jmessage;
        if (!reader.parse(it->ToJson(), jmessage)) {
            RTC_LOG(WARNING) << "stats report invalid!!!";
            return;
        }

        std::string type = jmessage["type"].asString();
        if (type == "outbound-rtp") {

            uint64_t rtt_ms = jmessage["rttMs"].asUInt64();
            uint64_t packetsLost = jmessage["packetsLost"].asUInt64();
            double fractionLost = jmessage["fractionLost"].asDouble();

            if (KRTCGlobal::Instance()->engine_observer()) {
                KRTCGlobal::Instance()->engine_observer()->OnNetworkInfo(rtt_ms, packetsLost, fractionLost);
            }
        }
    }
}

void KRTCPushImpl::handleHttpPushResponse(const HttpReply &reply) {
    if (reply.get_status_code() != 200 || reply.get_errno() != 0) {
        RTC_LOG(INFO) << "http post error";

        if (KRTCGlobal::Instance()->engine_observer()) {
            KRTCGlobal::Instance()->engine_observer()->OnPushFailed(KRTCError::kSendOfferErr);
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
        RTC_LOG(WARNING) << "Received unknown message. " << responseBody;

        if (KRTCGlobal::Instance()->engine_observer()) {
            KRTCGlobal::Instance()->engine_observer()->OnPushFailed(KRTCError::kParseAnswerErr);
        }
        return;
    }

    RTC_LOG(INFO) << "http push response : " << responseBody;

    int code = root["code"].asInt();
    if (code != 0) {
        RTC_LOG(INFO) << "http push response error, code: " << code;

        if (KRTCGlobal::Instance()->engine_observer()) {
            KRTCGlobal::Instance()->engine_observer()->OnPushFailed(KRTCError::kAnswerResponseErr);
        }
        return;
    }

    std::string sdpAnswer = root["sdp"].asString();

    webrtc::SdpParseError error;
    webrtc::SdpType type = webrtc::SdpType::kAnswer;
    std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
        webrtc::CreateSessionDescription(type, sdpAnswer, &error);

    peer_connection_->SetRemoteDescription(
        DummySetSessionDescriptionObserver::Create(),
        session_description.release());

    if (KRTCGlobal::Instance()->engine_observer()) {
        KRTCGlobal::Instance()->engine_observer()->OnPushSuccess();
    }

    assert(stats_timer_ == nullptr);
    stats_timer_ = std::make_unique<CTimer>(1 * 1000, true, [this]() {
          GetRtcStats();
    });
    stats_timer_->Start();
}

} // namespace krtc
