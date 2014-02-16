#include "graphdata.h"
#include <QString>

GraphData::GraphData()
{

}
bool GraphData::createNew(QString name)
{
    if(findByName(name)!=-1)
        return 0;
	file_name<<name;
    current_wheel_angle<<"0";
    desired_wheel_angle<<"0";
    wheel_power_r<<"0";
    wheel_power_l<<"0";
    physics_timestep<<"0";
    control_interval<<"0";
    line_position<<"0";
    return 1;
}
int GraphData::length()
{
    return file_name.length();
}
int GraphData::findByName(QString name)
{ 
    for (int i = 0; i < file_name.length(); ++i)
	{
		if(file_name[i]==name)
			return i;
    }
    return -1;
}
void GraphData::addTo(QString name, QString current_wheel_angle_1, QString desired_wheel_angle_1, QString wheel_power_r_1, QString wheel_power_l_1, QString physics_timestep_1, QString control_interval_1, QString line_position_1)
{
	int i = findByName(name);
    if ((current_wheel_angle_1=="nan")||(current_wheel_angle_1=="inf")||(current_wheel_angle_1=="-inf"))
        current_wheel_angle_1="null";
    if ((desired_wheel_angle_1=="nan")||(desired_wheel_angle_1=="inf")||(desired_wheel_angle_1=="-inf"))
        desired_wheel_angle_1="null";
    if ((wheel_power_r_1=="nan")||(wheel_power_r_1=="inf")||(wheel_power_r_1=="-inf"))
        wheel_power_r_1="null";
    if ((wheel_power_l_1=="nan")||(wheel_power_l_1=="inf")||(wheel_power_l_1=="-inf"))
        wheel_power_l_1="null";
    if ((physics_timestep_1=="nan")||(physics_timestep_1=="inf")||(physics_timestep_1=="-inf"))
        physics_timestep_1="null";
    if ((control_interval_1=="nan")||(control_interval_1=="inf")||(control_interval_1=="-inf"))
        control_interval_1="null";
    if ((line_position_1=="nan")||(line_position_1=="inf")||(line_position_1=="-inf")||(line_position_1=="-1"))
        line_position_1="null";
    current_wheel_angle[i]=current_wheel_angle[i]+", "+current_wheel_angle_1;
    desired_wheel_angle[i]=desired_wheel_angle[i]+", "+desired_wheel_angle_1;
    wheel_power_r[i]=wheel_power_r[i]+", "+wheel_power_r_1;
    wheel_power_l[i]=wheel_power_l[i]+", "+wheel_power_l_1;
    physics_timestep[i]=physics_timestep[i]+", "+physics_timestep_1;
    control_interval[i]=control_interval[i]+", "+control_interval_1;
    line_position[i]=line_position[i]+", "+line_position_1;
}
QString GraphData::get_name(int index)
{
    return file_name[index];
}
QString GraphData::get_current_wheel_angle(QString name)
{
    return get_current_wheel_angle(findByName(name));
}
QString GraphData::get_current_wheel_angle(int index)
{
    return current_wheel_angle[index];
}
QString GraphData::get_desired_wheel_angle(QString name)
{
    return get_desired_wheel_angle(findByName(name));
}
QString GraphData::get_desired_wheel_angle(int index)
{
    return desired_wheel_angle[index];
}
QString GraphData::get_wheel_power_r(QString name)
{
    return get_wheel_power_r(findByName(name));
}
QString GraphData::get_wheel_power_r(int index)
{
    return wheel_power_r[index];
}
QString GraphData::get_wheel_power_l(QString name)
{
    return get_wheel_power_l(findByName(name));
}
QString GraphData::get_wheel_power_l(int index)
{
    return wheel_power_l[index];
}
QString GraphData::get_physics_timestep(QString name)
{
    return get_physics_timestep(findByName(name));
}
QString GraphData::get_physics_timestep(int index)
{
    return physics_timestep[index];
}
QString GraphData::get_control_interval(QString name)
{
    return get_control_interval(findByName(name));
}
QString GraphData::get_control_interval(int index)
{
    return control_interval[index];
}
QString GraphData::get_line_position(QString name)
{
    return get_line_position(findByName(name));
}
QString GraphData::get_line_position(int index)
{
    return line_position[index];
}
QString GraphData::get(int index,int count)
{
    switch(count)
    {
    case 0:return current_wheel_angle[index];
    case 1:return desired_wheel_angle[index];
    case 2:return wheel_power_r[index];
    case 3:return wheel_power_l[index];
    case 4:return physics_timestep[index];
    case 5:return control_interval[index];
    case 6:return line_position[index];
    default: return "-1";
    }
}

void GraphData::deleteByName(QString name)
{
    int index=findByName(name);
    current_wheel_angle.removeAt(index);
    desired_wheel_angle.removeAt(index);
    wheel_power_r.removeAt(index);
    wheel_power_l.removeAt(index);
    physics_timestep.removeAt(index);
    control_interval.removeAt(index);
    line_position.removeAt(index);
    file_name.removeAt(index);
}
