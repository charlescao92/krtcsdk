#include "external_video_encoder_factory.h"
#include "absl/memory/memory.h"
#include "media/engine/internal_decoder_factory.h"
#include "rtc_base/logging.h"
#include "absl/strings/match.h"
#include "modules/video_coding/codecs/h264/include/h264.h"
#include "modules/video_coding/codecs/vp8/include/vp8.h"
#include "modules/video_coding/codecs/vp9/include/vp9.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "api/video_codecs/sdp_video_format.h"

#include "nv_encoder.h"
#include "qsv_encoder.h"
#include "x265_encoder.h"

namespace krtc {

	class ExternalEncoderFactory : public webrtc::VideoEncoderFactory {
	public:
		std::vector<webrtc::SdpVideoFormat> GetSupportedFormats()
			const override {
			std::vector<webrtc::SdpVideoFormat> video_formats;
			video_formats.push_back(webrtc::SdpVideoFormat(cricket::kH265CodecName));
			return video_formats;
		}

		VideoEncoderFactory::CodecInfo QueryVideoEncoder(
			const webrtc::SdpVideoFormat& format) const override {
			// Format must be one of the internal formats.
			RTC_DCHECK(
				format.IsCodecInList(GetSupportedFormats()));
			VideoEncoderFactory::CodecInfo info;
			return info;
		}

		std::unique_ptr<webrtc::VideoEncoder> CreateVideoEncoder(
			const webrtc::SdpVideoFormat& format) override {
			return absl::make_unique<krtc::X265Encoder>(cricket::VideoCodec(format));
		}
	};

	std::unique_ptr<webrtc::VideoEncoderFactory> CreateBuiltinExternalVideoEncoderFactory() {
		return absl::make_unique<ExternalEncoderFactory>();
	}

}
