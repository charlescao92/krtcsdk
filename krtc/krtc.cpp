#include <map>

#include <rtc_base/logging.h>
#include <rtc_base/task_utils/to_queued_task.h>

#include "krtc/krtc.h"
#include "krtc/base/krtc_global.h"
#include "krtc/media/krtc_pusher.h"
#include "krtc/media/krtc_puller.h"
#include "krtc/media/krtc_preview.h"
#include "krtc/device/camera_video_source.h"
#include "krtc/device/desktop_video_source.h"
#include "krtc/device/mic_impl.h"

namespace krtc {
    typedef std::map<const KRTCError, std::string> KRTCErrStrs;
    KRTCErrStrs KRTCErrStr = {
        {KRTCError::kNoErr, "NoErr"},
        {KRTCError::kVideoCreateCaptureErr,         "VideoCreateCaptureErr"},
        {KRTCError::kVideoNoCapabilitiesErr,        "VideoNoCapabilitiesErr"},
        {KRTCError::kVideoNoBestCapabilitiesErr,    "VideoNoBestCapabilitiesErr"},
        {KRTCError::kVideoStartCaptureErr,          "VideoStartCaptureErr"},
        {KRTCError::kPreviewNoVideoSourceErr,       "PreviewNoVideoSourceErr"},
        {KRTCError::kInvalidUrlErr,                 "InvalidUrlErr"},
        {KRTCError::kAddTrackErr,                   "AddTrackErr"},
        {KRTCError::kCreateOfferErr,                "CreateOfferErr"},
        {KRTCError::kSendOfferErr,                  "SendOfferErr"},
        {KRTCError::kParseAnswerErr,                "ParseAnswerErr"},
        {KRTCError::kAnswerResponseErr,             "kAnswerResponseErr"},
        {KRTCError::kNoAudioDeviceErr,              "NoAudioDeviceErr"},
        {KRTCError::kAudioNotFoundErr,              "AudioNotFoundErr"},
        {KRTCError::kAudioSetRecordingDeviceErr,    "AudioSetRecordingDeviceErr"},
        {KRTCError::kAudioInitRecordingErr,	        "AudioInitRecordingErr"},
        {KRTCError::kAudioStartRecordingErr,        "AudioStartRecordingErr"},
    };

    void KRTCEngine::Init(CONTROL_TYPE type, KRTCEngineObserver* observer) {
        rtc::LogMessage::LogTimestamps(true);
        rtc::LogMessage::LogThreads(true);
        rtc::LogMessage::LogToDebug(rtc::LS_VERBOSE);

        KRTCGlobal::Instance()->RegisterEngineObserver(observer);

        RTC_LOG(LS_INFO) << "KRTCSDK init";
        switch (type)
        {
        case krtc::CONTROL_TYPE::PUSH:
            KRTCGlobal::Instance()->InitPush();
            break;
        case krtc::CONTROL_TYPE::PULL:
            break;
        case krtc::CONTROL_TYPE::BOTH:
            KRTCGlobal::Instance()->InitPush();
            break;
        default:
            break;
        }    
    }

    const char* KRTCEngine::GetErrString(const KRTCError& err) {
        return KRTCErrStr[err].c_str();
    }

    uint32_t KRTCEngine::GetCameraCount() {
        return KRTCGlobal::Instance()->api_thread()->Invoke<uint32_t>(RTC_FROM_HERE, [=]() {
            return KRTCGlobal::Instance()->video_device_info()->NumberOfDevices();
        });
        return 0;
    }

    int32_t KRTCEngine::GetCameraInfo(int index, char* device_name, uint32_t device_name_length,
        char* device_id, uint32_t device_id_length)
    {
        assert(device_name_length == 128 && device_id_length == 128);
        return KRTCGlobal::Instance()->api_thread()->Invoke<int32_t>(RTC_FROM_HERE, [&]() {
            int32_t res = KRTCGlobal::Instance()->video_device_info()->GetDeviceName(index,
                device_name, device_name_length, device_id, device_id_length);
            return res;
            });
        return 0;
    }

    IVideoHandler* KRTCEngine::CreateCameraSource(const char* cam_id) {
        return KRTCGlobal::Instance()->api_thread()->Invoke<IVideoHandler*>(RTC_FROM_HERE, [=]() {
            return new CameraVideoSource(cam_id);
        });
        return nullptr;
    }

    uint32_t KRTCEngine::GetScreenCount()
    {
        return KRTCGlobal::Instance()->api_thread()->Invoke<int32_t>(RTC_FROM_HERE, [=]() {
            int32_t res = KRTCGlobal::Instance()->GetScreenCount();
            return res;
            });
        return 0;
    }

    IVideoHandler* KRTCEngine::CreateScreenSource(const uint32_t& screen_index)
    {
        return KRTCGlobal::Instance()->api_thread()->Invoke<IVideoHandler*>(RTC_FROM_HERE, [=]() {
            return new DesktopVideoSource(screen_index);
            });
        return nullptr;
    }

    int16_t KRTCEngine::GetMicCount() {
        return KRTCGlobal::Instance()->api_thread()->Invoke<uint32_t>(RTC_FROM_HERE, [=]() {
            return KRTCGlobal::Instance()->audio_device()->RecordingDevices();
            });
        return 0;
    }

    int32_t KRTCEngine::GetMicInfo(int index, char* mic_name, uint32_t mic_name_length,
        char* mic_guid, uint32_t mic_guid_length) 
    {
        assert(mic_name_length == 128 && mic_guid_length == 128);
        return KRTCGlobal::Instance()->api_thread()->Invoke<int32_t>(RTC_FROM_HERE, [&]() {
            int32_t ret = KRTCGlobal::Instance()->audio_device()->RecordingDeviceName(
                index, mic_name, mic_guid);
            return ret;
        });
        return 0;
    }

    IAudioHandler* KRTCEngine::CreateMicSource(const char* mic_id) {
       return KRTCGlobal::Instance()->api_thread()->Invoke<IAudioHandler*>(RTC_FROM_HERE, [=]() {
            return new MicImpl(mic_id);
        });
        return nullptr;
    }

    IMediaHandler* KRTCEngine::CreatePreview(int hwnd) {
       return KRTCGlobal::Instance()->api_thread()->Invoke<IMediaHandler*>(RTC_FROM_HERE, [=]() {
            return new KRTCPreview(hwnd);
       });
        return nullptr;
    }

    IMediaHandler* KRTCEngine::CreatePusher(const char* server_addr, const char* push_channel) {
       return KRTCGlobal::Instance()->api_thread()->Invoke<IMediaHandler*>(RTC_FROM_HERE, [=]() {
            return new KRTCPusher(server_addr, push_channel);
       });
       return nullptr;
    }

    IMediaHandler* KRTCEngine::CreatePuller(const char* server_addr, const char* pull_channel, int hwnd) {
        return KRTCGlobal::Instance()->api_thread()->Invoke<IMediaHandler*>(RTC_FROM_HERE, [=]() {
            return new KRTCPuller(server_addr, pull_channel, hwnd);
        });
        return nullptr;
    }

} // end namespace KRTC
