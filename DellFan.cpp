// #include "FindHwmon.h"
#include <stdio.h>
#include <iostream>
#include <string>
// #include <cstring>
#include <algorithm>  // std::max
#include <fstream>    //read  and write files
#include <filesystem> // file browsing
#include <unistd.h>   // sleep library
// #include <stdexcept> //error handling
// #include <gtk/gtk.h> // gtk interface

namespace fs = std::filesystem;

const std::string hwmon = "/sys/class/hwmon";
int cpu_temp;
int gpu_temp;
int cpu_fan;
int gpu_fan;
std::string dellsmm;
std::string dGPU;
std::string CPU; // either k10temp or zenpower

//  Gets the Hwmon ids of dellsmm, k10temp/zenpower, and dGPU.
void Hwmon_get()
{
    // std::string dellsmm_path = "";
    // std::string cpu_path = "";
    for (const auto &entry : fs::directory_iterator(hwmon))
    {
        std::ifstream namepath = std::ifstream(entry.path().string() + "/name");
        std::string name = std::string((std::istreambuf_iterator<char>(namepath)),
                                       (std::istreambuf_iterator<char>()));
        if (name == "dell_smm\n")
        {
            // dellsmm_path = entry.path().string().back();
            dellsmm = entry.path().string();
        }
        else if (name == "zenpower\n")
        {
            // cpu_path = entry.path().string().back();
            CPU = entry.path().string();
        }
        else if (name == "k10temp\n")
        {
            // cpu_path = entry.path().string().back();
            CPU = entry.path().string();
        }
        else if (name == "amdgpu\n")
        {
            // There are two amd gpus (iGPU and dGPU) an easy way to differentiate them is to check the presence of pwm1. Another way would be to check vbios version (may be more robust).
            if (fs::exists(entry.path().string() + "/pwm1"))
            {
                dGPU = entry.path().string();
            }
        }
    }
};

// Updates the thermals and fan variables.
void update_vars()
{
    std::ifstream a;
    a.open(CPU + "/temp2_input"); //Tdie cpu temp
    a >> cpu_temp;
    a.close();
    a.open(dGPU + "/temp2_input"); //Junction dGPU temp
    a >> gpu_temp;
    a.close();
    a.open(dellsmm + "/fan1_input"); //Processor fan
    a >> cpu_fan;
    a.close();
    a.open(dellsmm + "/fan3_input"); // Video fan
    a >> gpu_fan;
    a.close();
};

// Set fans to selected speed.

void set_cpu_fan(int left)
{
    // Force left to be in [0,256]
    int l = std::max(0, std::min(256, left));
    // Writes to hwmon
    std::ofstream pwm;
    pwm.open(dellsmm + "/pwm1");
    pwm << l;
    pwm.close();
};
void set_gpu_fan(int right)
{
    // Force right to be in [0,256]
    int r = std::max(0, std::min(256, right));
    // Writes to hwmon
    std::ofstream pwm;
    pwm.open(dellsmm + "/pwm3");
    pwm << r;
    pwm.close();
};

void check_fan_write_permission()
{
    std::ofstream pwm;
    pwm.open(dellsmm + "/pwm1");
    if (!pwm.is_open())
    {
        std::cout << "Cannot change fan speed. Are you running the script with root permission ?" << std::endl;
        exit(EXIT_FAILURE);
    }
    pwm.close();
};

// Updates fans accordings to temp.
void update_fans(int lowtemp, int hightemp)
{
    // int fan_update[2] = {-1, -1}; //To debug
    // Handle the left (cpu) fan
    if (cpu_temp < lowtemp)
    {
        if (cpu_fan > 2500)
        {
            set_cpu_fan(0);
            // new_fan_values[0] = 0;
        }
    }
    else if (cpu_temp < hightemp)
    {
        if (cpu_fan <= 2000 || cpu_fan >= 3500)
        {
            set_cpu_fan(128);
            // new_fan_values[0] = 128
        }
    }
    else
    {
        if (cpu_fan < 3500)
        {
            set_cpu_fan(255);
            // new_fan_values[0] = 255;
        }
    }
    // Handles the right (GPU) fan
    if (gpu_temp < lowtemp)
    {
        if (gpu_fan > 2500)
        {
            set_gpu_fan(0);
            // new_fan_values[1] = 0;
        }
    }
    else if (gpu_temp < hightemp)
    {
        if (gpu_fan <= 2000 || gpu_fan >= 3500)
        {
            set_gpu_fan(128);
            // new_fan_values[1] = 128;
        }
    }
    else
    {
        if (gpu_fan < 3500)
        {
            set_gpu_fan(255);
            // new_fan_values[1] = 255;
        }
    }
};

void print_status()
{
    std::cout << "Current fan speeds : " << cpu_fan << " RPM and " << gpu_fan << " RPM.                 " << std::endl;
    std::cout << "CPU and GPU temperatures : " << cpu_temp/1000 << "°C and " << gpu_temp/1000 << "°C.                 " << std::endl;
    std::cout << "\033[2F";
};

int main(int argc, char* argv[])
{

    if (argc <= 2)
    {
        printf("Need more arguments.");
        exit(EXIT_FAILURE);
    }

    const int lowtemp = std::stoi(argv[1]) * 1000;
    const int hightemp = std::stoi(argv[2]) * 1000;
    int timer =5;
    if (argc >3){
        timer = std::stoi(argv[3]);
    }
    // std::cout << "Script launched with arguments : " << lowtemp/1000 << " " << hightemp/1000 << " " << timer <<std::endl; //Debug
    
    // Get hwmon variables
    Hwmon_get();
    // Check if launched with enough permissions.
    check_fan_write_permission();
    
    // Fan update loop
    while (true)
    {
        //First update the variables.
        update_vars();
        //Then update the fan speed accordingly.
        update_fans(lowtemp,hightemp);
        //Prints current status
        print_status();
        // wait $timer seconds
        sleep(timer);
    }
    return 0;
}

// void on_window_main_destroy()
// {
//     gtk_main_quit();
// }