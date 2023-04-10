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
#include "krtc/device/krtc_audio_track.h"
#include "krtc/tools/timer.h"

namespace krtc {

KRTCPushImpl::KRTCPushImpl(const std::string& server_addr, const std::string& push_channel) :
    KRTCMediaBase(STREAM_CONTROL_TYPE::PUSH_TYPE, server_addr, push_channel){}

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

    // 创建音频源
    cricket::AudioOptions options;
    rtc::scoped_refptr<LocalAudioSource> audio_source(LocalAudioSource::Create(&options));

    // 添加音频轨
    rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
            peer_connection_factory->CreateAudioTrack(kAudioLabel, audio_source));
    auto add_audio_track_result = peer_connection_->AddTrack(audio_track, { kStreamId });
    if (!add_audio_track_result.ok()) {
        RTC_LOG(LS_ERROR) << "Failed to add audio track to PeerConnection: "
            << add_audio_track_result.error().message();
    }

    // 添加视频轨
    rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track(
        peer_connection_factory->CreateVideoTrack(
            kAudioLabel, KRTCGlobal::Instance()->current_video_source()));
    auto add_video_track_result = peer_connection_->AddTrack(video_track, { kStreamId });
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
    if (peer_connection_) {
        peer_connection_ = nullptr;
    }

    if (stats_timer_) {
        stats_timer_->Stop();
        stats_timer_ = nullptr;
    }
}

void KRTCPushImpl::GetRtcStats() {
    KRTCGlobal::Instance()->api_thread()->PostTask(webrtc::ToQueuedTask([=]() {
        if (!peer_connection_) {
            return;
        }
        RTC_LOG(LS_INFO) << "KRTCPushImpl  GetRtcStats";
        if (!stats_) {
            stats_ = new rtc::RefCountedObject<CRtcStatsCollector>(this);
        }
        peer_connection_->GetStats(stats_.get());
     }));
}

void KRTCPushImpl::OnDataChannel(
    rtc::scoped_refptr<webrtc::DataChannelInterface> channel) {
    RTC_LOG(INFO) << __FUNCTION__;
}

// CreateSessionDescriptionObserver implementation.
void KRTCPushImpl::OnSuccess(webrtc::SessionDescriptionInterface* desc) {
    RTC_LOG(INFO) << __FUNCTION__;

    peer_connection_->SetLocalDescription(
        DummySetSessionDescriptionObserver::Create(), desc);

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

    RTC_LOG(LS_INFO) << "send webrtc push request.....";

    std::string responseBody;
    Http::HTTPERROR errCode = httpExecRequest("POST", httpRequestUrl_, responseBody, json_data);
    if (errCode != Http::HTTPERROR_SUCCESS) {
        RTC_LOG(INFO) << "http post error : " << httpErrorString(errCode);

        if (KRTCGlobal::Instance()->engine_observer()) {
            KRTCGlobal::Instance()->engine_observer()->OnPushFailed(KRTCError::kSendOfferErr);
        }
        return;
    }

    // 解析srs服务器发回来的json数据
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

    // 开始获取推流状态信息
    assert(stats_timer_ == nullptr);
    stats_timer_ = std::make_unique<CTimer>(1 * 1000, true, [this]() {
          GetRtcStats(); 
    });
    stats_timer_->Start();
}

void KRTCPushImpl::OnFailure(webrtc::RTCError error) {
    RTC_LOG(INFO) << __FUNCTION__;

    if (KRTCGlobal::Instance()->engine_observer()) {
        KRTCGlobal::Instance()->engine_observer()->OnPushFailed(KRTCError::kCreateOfferErr);
    }
}

void KRTCPushImpl::OnStatsInfo(const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report)
{
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

} // namespace krtc
