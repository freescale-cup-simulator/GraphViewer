#ifndef GRAPHDATA_H
#define GRAPHDATA_H
#include <QString>
#include <QStringList>

class GraphData
{

    QStringList camera_pixels;
    QStringList current_wheel_angle;
    QStringList desired_wheel_angle;
    QStringList wheel_power_r;
    QStringList wheel_power_l;
    QStringList physics_timestep;
    QStringList control_interval;
    QStringList line_position;
    QStringList file_name;
    int findByName(QString name);
public:
    GraphData();
    bool createNew(QString name);
    void addTo(QString name, QString current_wheel_angle, QString desired_wheel_angle, QString wheel_power_r, QString wheel_power_l, QString physics_timestep, QString control_interval, QString line_position);
    int length();
    QString get_name(int index);
    QString get_current_wheel_angle(QString name);
    QString get_current_wheel_angle(int index);
    QString get_desired_wheel_angle(QString name);
    QString get_desired_wheel_angle(int index);
    QString get_wheel_power_r(QString name);
    QString get_wheel_power_r(int index);
    QString get_wheel_power_l(QString name);
    QString get_wheel_power_l(int index);
    QString get_physics_timestep(QString name);
    QString get_physics_timestep(int index);
    QString get_control_interval(QString name);
    QString get_control_interval(int index);
    QString get_line_position(QString name);
    QString get_line_position(int index);
    QString get(int index,int count);
    void deleteByName(QString name);
};

#endif // GRAPHDATA_H
