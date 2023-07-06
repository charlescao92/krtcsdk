#include "external_video_decoder_factory.h"
#include "absl/memory/memory.h"
#include "media/engine/internal_decoder_factory.h"
#include "rtc_base/logging.h"
#include "absl/strings/match.h"
#include "modules/video_coding/codecs/h265/include/h265.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "api/video_codecs/sdp_video_format.h"

#include "x265_decoder.h"

namespace krtc {

	class ExternalDecoderFactory : public webrtc::VideoDecoderFactory {
	public:
		std::vector<webrtc::SdpVideoFormat> GetSupportedFormats()
			const override {
			std::vector<webrtc::SdpVideoFormat> video_formats;
			video_formats.push_back(webrtc::SdpVideoFormat(cricket::kH265CodecName));
			return video_formats;
		}

		std::unique_ptr<webrtc::VideoDecoder> CreateVideoDecoder(
			const webrtc::SdpVideoFormat& format) override {
			return absl::make_unique<krtc::H265DecoderImpl>(cricket::VideoCodec(format));
		}
	};

	std::unique_ptr<webrtc::VideoDecoderFactory> CreateBuiltinExternalVideoDecoderFactory() {
		return absl::make_unique<ExternalDecoderFactory>();
	}

}
