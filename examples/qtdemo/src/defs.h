#ifndef DEFS_H
#define DEFS_H

#include <string>

#include <QMetaType>

struct DeviceInfo
{
    std::string device_name;
    std::string device_id;
};
Q_DECLARE_METATYPE(DeviceInfo);

#endif // DEFS_H
