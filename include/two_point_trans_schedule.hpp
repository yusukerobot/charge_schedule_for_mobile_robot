#pragma once

#include <vector>
#include <string>

#include "nsgaii.hpp"

namespace charge_schedule
{
    class TwoTransProblem : public nsgaii::ScheduleNsgaii
    {
    public:
        TwoTransProblem(const std::string& config_file_path);
        ~TwoTransProblem() override = default;

        nsgaii::Individual generateIndividual();

        void generateFirstCombinedPopulation() override;
        void evaluatePopulation(std::vector<nsgaii::Individual>& population) override;

        void calucObjectiveFunction(nsgaii::Individual& individual);
        float makespan(std::vector<std::array<float, 4>>& T_span);
        float soc_HiLowTime(std::vector<std::array<float, 2>> T_SOC_HiLow);

        void calcSOCHiLow(nsgaii::Individual& individual);
        float calcChargingTime(int& soc_start, int& soc_target);

        void testTwenty();
        
    private:
        int min_charge_number;        // 最小充電回数
        float T_cycle;  // 1回のタスクにかかる時間
        float E_cycle;  // 1回のタスクの放電量
    };
} // namespace charge_schedule
