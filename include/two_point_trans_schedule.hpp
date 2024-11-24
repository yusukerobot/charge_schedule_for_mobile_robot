#pragma once

#include <vector>
#include <string>

#include "nsgaii.hpp"

namespace charge_schedule
{
    class TowTransProblem : public nsgaii::ScheduleNsgaii
    {
    public:
        TowTransProblem(const std::string& config_file_path);

        void generateFirstCombinedPopulation();
        void evaluatePopulation(std::vector<nsgaii::Individual>& population);

        void calucObjectiveFunction(nsgaii::Individual& individual);
        float makespan(std::vector<float>& T_span);
        float soc_HiLowTime(std::vector<float>& T_socHi, std::vector<float>& T_socLow);
        
    private:
        
    };
} // namespace charge_schedule
