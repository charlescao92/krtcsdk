#ifndef KRTCSDK_KRTC_CODEC_EXTERNAL_BUILTIN_VIDEO_DECODER_FACTORY_H_
#define KRTCSDK_KRTC_CODEC_EXTERNAL_BUILTIN_VIDEO_DECODER_FACTORY_H_

#include <memory>

#include "rtc_base/system/rtc_export.h"
#include "api/video_codecs/video_decoder_factory.h"

namespace krtc {

	// Creates a new factory that can create the built-in types of video decoders.
	std::unique_ptr<webrtc::VideoDecoderFactory> CreateBuiltinExternalVideoDecoderFactory();

} // namespace krtc

#endif  // KRTCSDK_KRTC_CODEC_EXTERNAL_BUILTIN_VIDEO_DECODER_FACTORY_H_