#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <math.h>

// class CenterPoint
// {
//     public:
//         CenterPoint(double x, double y): x_(x), y_(y){}

//     public:
//         
// };

class Vehicle
{
    public:
        Vehicle(double x,double y,double l, double w, double vx, double vy, double acc, int lanep, double theta_r, double theta_a, double q) 
        : x_(x), y_(y), length_(l), width_(w), speed_x_(vx), speed_y_(vy), acc_(acc), lane_posi_(lanep), relative_theta_(theta_r), absolute_theta_(theta_a), init_q_(q)
        {
            speed_ = sqrt(pow(speed_x_, 2) + pow(speed_y_, 2));
        }
        Vehicle(){}

    public:
        double x_;           //车辆横坐标
        double y_;           //车辆纵坐标
        double length_;      //车长
        double width_;       //车宽
        double speed_;       //车速：车辆纵向
        double speed_x_;     //绝对坐标系下x轴向速度
        double speed_y_;     //绝对坐标系下y轴纵向速度
        double acc_;         //车辆加加速度：车辆纵向
        int lane_posi_;      //参考路径所在的车道数,具体编号见下图（三车道为例）
                             //    |     |     |     |
                             //    |  ^  |  ^  |  ^  |
                             //    |  ^  |  ^  |  ^  |
                             //    |  ^  |  ^  |  ^  |
                             //    |  ^  |  ^  |  ^  |
                             //    |     |     |     |
                             //    4     3     2     1                                       
        double relative_theta_;  //车辆相对于路径的偏角，逆时针为正  单位：rad
        double absolute_theta_;  //车辆相在绝对坐标系下相对于y轴的偏角，逆时针为正  单位：rad
        double init_q_;          //车辆相对于参考路径的偏移量，车辆在参考路径左边，横向偏移量为正
};

class surVehicle
{
    public:
        surVehicle(double x, double y, double l, double w, double v_long, double v_lat, double acc_long, double acc_lat, double heading)
                    : x_(x), y_(y), length_(l), width_(w), speed_x_(v_long), speed_y_(v_lat), acc_x_(acc_long), acc_y_(acc_lat), heading_(heading)
        {}
        surVehicle(){}

    public:
        double x_;           //背景车x坐标
        double y_;           //背景车y坐标
        double length_;      //车长
        double width_;       //车宽
        double speed_x_;     //绝对坐标系下x轴向速度
        double speed_y_;     //绝对坐标系下y轴向速度
        double acc_x_;       //绝对坐标系下x轴向加速度
        double acc_y_;       //绝对坐标系下x轴向加速度
        double heading_;     //绝对坐标系下背景车相对于y轴角度，逆时针为正  单位：rad

};