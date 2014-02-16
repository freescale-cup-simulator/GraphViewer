#ifndef COMMON_H
#define COMMON_H

#include <QVector3D>
#include <QQuaternion>
#include <QDebug>

constexpr int CAMERA_FRAME_LEN = 128;
constexpr quint32 DATASET_VERSION=0x1;

typedef struct
{
    QVector3D p;
    QQuaternion q;
} BodyData;

typedef struct
{
    BodyData camera;
    BodyData vehicle;
    BodyData wheels[4];

    quint8 camera_pixels[CAMERA_FRAME_LEN];
    float current_wheel_angle;
    float desired_wheel_angle;
    float wheel_power_r;
    float wheel_power_l;
    float physics_timestep;
    float control_interval;
    qint32 line_position;
} DataSet;

Q_DECLARE_METATYPE(DataSet)

#endif // COMMON_H
