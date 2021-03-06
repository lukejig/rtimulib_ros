// RTIMULib ROS Node
// Copyright (c) 2015, Romain Reignier
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//   * Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   * Neither the name of the <organization> nor the
//     names of its contributors may be used to endorse or promote products
//     derived from this software without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <RTIMULib.h>
#include <ros/ros.h>
#include <sensor_msgs/Imu.h>
#include <sensor_msgs/MagneticField.h>

static const double G_TO_MPSS = 9.80665;
#define uT_2_T 1000000

int main(int argc, char **argv)
{
    ros::init(argc, argv, "rtimulib_node");
    ROS_INFO("Imu driver is running");
    ros::NodeHandle n;
    ros::Publisher imu_pub = n.advertise<sensor_msgs::Imu>("imu/data", 1);
    ros::Publisher magnetometer_pub_ = n.advertise<sensor_msgs::MagneticField>("imu_mag", 1);

    std::string path_calib_file;
    n.getParam("/rtimulib_node/calibration_file_path", path_calib_file);
    std::string frame_id;
    n.getParam("/rtimulib_node/frame_id", frame_id);
    double update_rate;
    if(!n.getParam("/rtimulib_node/update_rate", update_rate))
    {
        ROS_WARN("No update_rate provided - default: 20 Hz");
        update_rate = 20;
    }

    // Load the RTIMULib.ini config file
    RTIMUSettings *settings = new RTIMUSettings(path_calib_file.c_str(), "RTIMULib"); 

    RTIMU *imu = RTIMU::createIMU(settings);

    if ((imu == NULL) || (imu->IMUType() == RTIMU_TYPE_NULL))
    {
        ROS_ERROR("No Imu found");
        return -1;
    }

    // Initialise the imu object
    imu->IMUInit();

    // Set the Fusion coefficient
    imu->setSlerpPower(0.02);
    // Enable the sensors
    imu->setGyroEnable(true);
    imu->setAccelEnable(true);
    imu->setCompassEnable(true);

    ros::Rate loop_rate(update_rate);
    while (ros::ok())
    {
        sensor_msgs::Imu imu_msg;
        sensor_msgs::MagneticField imu_mage_msg;

        if (imu->IMURead())
        {
            RTIMU_DATA imu_data = imu->getIMUData();
            imu_msg.header.stamp = ros::Time::now();
            imu_msg.header.frame_id = frame_id;
            imu_msg.orientation.x = imu_data.fusionQPose.x(); 
            imu_msg.orientation.y = imu_data.fusionQPose.y(); 
            imu_msg.orientation.z = imu_data.fusionQPose.z(); 
            imu_msg.orientation.w = imu_data.fusionQPose.scalar(); 
            imu_msg.angular_velocity.x = imu_data.gyro.x();
            imu_msg.angular_velocity.y = imu_data.gyro.y();
            imu_msg.angular_velocity.z = imu_data.gyro.z();
            imu_msg.linear_acceleration.x = imu_data.accel.x();
            imu_msg.linear_acceleration.y = imu_data.accel.y();
            imu_msg.linear_acceleration.z = imu_data.accel.z();

            imu_pub.publish(imu_msg);

            imu_mage_msg.header.frame_id=frame_id;
            imu_mage_msg.header.stamp=ros::Time::now();

            imu_mage_msg.magnetic_field.x = imu_data.compass.x()/uT_2_T;
            imu_mage_msg.magnetic_field.y = imu_data.compass.y()/uT_2_T;
            imu_mage_msg.magnetic_field.z = imu_data.compass.z()/uT_2_T;

            magnetometer_pub_.publish(imu_mage_msg);
        }
        ros::spinOnce();
    }
    return 0;
}
