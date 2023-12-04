#include "krtc/device/mic_impl.h"

#include <rtc_base/logging.h>
#include <api/audio/audio_frame.h>
#include <modules/audio_device/include/audio_device.h>
#include <api/peer_connection_interface.h>

#include "krtc/base/krtc_global.h"
#include "krtc/media/media_frame.h"

namespace krtc {

MicImpl::MicImpl(const char* mic_id) :
    mic_id_(mic_id),
    signaling_thread_(rtc::Thread::Current())
{
}

MicImpl::~MicImpl() {}

void MicImpl::Start() {
    RTC_LOG(LS_INFO) << "MicImpl Start call";

    KRTCGlobal::Instance()->worker_thread()->PostTask([=]() {

        RTC_LOG(LS_INFO) << "MicImpl Start PostTask";

        KRTCError err = KRTCError::kNoErr;

        do {

            // 1. 如果麦克风已经启动采集，直接停止
            if (has_start_) {
                RTC_LOG(LS_WARNING) << "mic already start, mic_id: " << mic_id_;
                break;
            }

            // 2. 直接从webrtc获取adm模块指针
            rtc::scoped_refptr<webrtc::AudioDeviceModule> audio_device =
               KRTCGlobal::Instance()->push_peer_connection_factory()->GetAdmPtr();


            // 3. 检查系统是否存在麦克风设备
            int total = audio_device->RecordingDevices();
            if (total <= 0) {
                RTC_LOG(LS_WARNING) << "no audio device";
                err = KRTCError::kNoAudioDeviceErr;
                break;
            }

            // 4. 检查关联的mic_id是否能够在系统设备中找到
            int device_index = -1;
            for (int i = 0; i < total; ++i) {
                char name[128];
                char guid[128];
                audio_device->RecordingDeviceName(i, name, guid);
                if (0 == strcmp(guid, mic_id_.c_str())) {
                    device_index = i;
                    break;
                }
            }

            if (device_index <= -1) {
                RTC_LOG(LS_WARNING) << "audio device not found, mic_id: " << mic_id_;
                err = KRTCError::kAudioNotFoundErr;
                break;
            }

            // 5. 设置启用的麦克风设备
            if (audio_device->SetRecordingDevice(device_index)) {
                RTC_LOG(LS_WARNING) << "SetRecordingDevice failed, mic_id: " << mic_id_;
                err = KRTCError::kAudioSetRecordingDeviceErr;
                break;
            }

            // 6. 设置为立体声采集
            audio_device->SetStereoRecording(true);

            // 7. 初始化麦克风
            if (audio_device->InitRecording() || !audio_device->RecordingIsInitialized()) {
                RTC_LOG(LS_WARNING) << "InitRecording failed, mic_id: " << mic_id_;
                err = KRTCError::kAudioInitRecordingErr;
                break;
            }

            bool ok = false;
            audio_device->PlayoutIsAvailable(&ok);
            if (!ok) {
                RTC_LOG(LS_WARNING) << "PlayoutIsAvailable failed, mic_id: " << mic_id_;
                err = KRTCError::kAudioInitRecordingErr;
                break;
            }

            int32_t ret = audio_device->InitPlayout();
            if (audio_device->StartPlayout()) {
                RTC_LOG(LS_WARNING) << "StartPlayout failed!!!";
                err = KRTCError::kAudioStartRecordingErr;
                break;
            }

            // 8. 启动麦克风采集
            if (audio_device->StartRecording()) {
                RTC_LOG(LS_WARNING) << "StartRecording failed, mic_id: " << mic_id_;
                err = KRTCError::kAudioStartRecordingErr;
                break;
            }

            has_start_ = true;

        } while (0);

        if (err == KRTCError::kNoErr) {
            if (KRTCGlobal::Instance()->engine_observer()) {
                KRTCGlobal::Instance()->engine_observer()->OnAudioSourceSuccess();
            }
        }
        else {
            if (KRTCGlobal::Instance()->engine_observer()) {
                KRTCGlobal::Instance()->engine_observer()->OnAudioSourceFailed(err);
            }
        }

    }); 
}

void MicImpl::Stop() {
    RTC_LOG(LS_INFO) << "MicImpl Stop call";
    KRTCGlobal::Instance()->worker_thread()->PostTask([=]() {
        RTC_LOG(LS_INFO) << "MicImpl Stop PostTask";
        if (!has_start_) {
            return;
        }

        has_start_ = false;

        webrtc::AudioDeviceModule* audio_device = KRTCGlobal::Instance()->audio_device(); 
        if (audio_device->RecordingIsInitialized()) {
            audio_device->StopRecording();
        }
        if (audio_device->PlayoutIsInitialized()) {
            audio_device->StopPlayout();
        }
    });
}

void MicImpl::Destroy() {
    RTC_LOG(LS_INFO) << "MicImpl Destroy call";

    KRTCGlobal::Instance()->worker_thread()->PostTask([=]() {
        RTC_LOG(LS_INFO) << "MicImpl Destroy PostTask";
        delete this;
    });     
}

} // namespace krtc
