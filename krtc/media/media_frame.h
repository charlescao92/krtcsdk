#ifndef KRTCSDK_KRTC_MEDIA_MEDIA_FRAME_H_
#define KRTCSDK_KRTC_MEDIA_MEDIA_FRAME_H_

#include "krtc/krtc.h"

namespace krtc {

    enum class MainMediaType {
        kMainTypeCommon,
        kMainTypeAudio,
        kMainTypeVideo,
        kMainTypeData   // �ı���Ϣ������ý����Ϣ����
    };

    enum class SubMediaType {
        kSubTypeCommon,
        kSubTypeI420,
        kSubTypeH264,
        kSubTypePcm,
        kSubTypeOpus,
    };

    struct AudioFormat {
        SubMediaType type;
        size_t nbytes_per_sample;   // �����ֽ�����16bit
        size_t samples_per_channel; // ÿ�������Ĳ�����
        size_t channels;            // ͨ����
        uint32_t samples_per_sec;   // ������
        uint32_t total_delay_ms;    // �ӳ�ʱ�䣬�����������õ�
        bool key_pressed;           // ���̰���������������õ�
    };

    struct KRTC_API VideoFormat {
        SubMediaType type;
        int width;
        int height;
        bool idr; // �Ƿ��ǹؼ�֡
    };

    class KRTC_API MediaFormat {
    public:
        MainMediaType media_type;
        union {
            AudioFormat audio_fmt;
            VideoFormat video_fmt;
        } sub_fmt;
    };

    class KRTC_API MediaFrame {
    public:
        MediaFrame(int size) : max_size(size) {
            memset(data, 0, sizeof(data));
            memset(data_len, 0, sizeof(data_len));
            memset(stride, 0, sizeof(stride));
            data[0] = new char[size];
            data_len[0] = size;
        }

        ~MediaFrame() {
            if (data[0]) {
                delete[] data[0];
                data[0] = nullptr;
            }
        }

    public:
        int max_size;                // �ṹ�������
        MediaFormat fmt;             // ý�����ͣ���Ƶ������Ƶ
        char* data[4];               // Ĭ����4��ƽ���������洢����
        int data_len[4];             // ÿ��ƽ��������ֽڴ�С
        int stride[4];               // ÿһ�еĴ�С
        uint32_t ts = 0;             // ֡��ʱ���
        int64_t capture_time_ms = 0; // �ɼ�ʱ��
    };

} // namespace krtc

#endif // KRTCSDK_KRTC_MEDIA_MEDIA_FRAME_H_