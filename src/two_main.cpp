#include <memory>
#include <iostream>
#include <yaml-cpp/yaml.h>

#include "two_point_trans_schedule.hpp"

int main()
{
   std::string config_file_path = "/home/robotdesktop/cpp/charge_schedule_for_mobile_robot/params/two_charge_schedule.yaml";
   std::unique_ptr<charge_schedule::TwoTransProblem> nsgaii = std::make_unique<charge_schedule::TwoTransProblem>(config_file_path);

   nsgaii->generateFirstCombinedPopulation();

   return 0;
}