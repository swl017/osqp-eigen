#ifndef HKMPC_H
#define HKMPC_H

#include "OsqpEigen/OsqpEigen.h"

// eigen
#include <Eigen/Dense>

#include <iostream>
#include <vector>

#define nx 12
#define nu 4


class hkmpc
{
private:
    
    struct Vehicle
    {
        double x;
        double y;
        double yaw;
        double vx;
        double steer;
        double accel;
    };

    struct ReferenceTrajectory
    {
        std::vector<double> path_x;
        std::vector<double> path_y;
        std::vector<double> path_vx;
        std::vector<double> path_yaw;
    };

    struct MPCParam
    {
        /* Vehicle Parameters */
        double l_f;
        double l_r;
    };
    

public:
    hkmpc(/* args */);
    ~hkmpc();
};









#endif HKMPC_H