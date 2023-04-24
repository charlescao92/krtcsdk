#ifndef DEFS_H
#define DEFS_H

#include <string>

#include <QMetaType>

#include "krtc/media/media_frame.h"

struct DeviceInfo
{
    std::string device_name;
    std::string device_id;
};
Q_DECLARE_METATYPE(DeviceInfo);

typedef QSharedPointer<krtc::MediaFrame> MediaFrameSharedPointer;
Q_DECLARE_METATYPE(MediaFrameSharedPointer);

#endif // DEFS_H
