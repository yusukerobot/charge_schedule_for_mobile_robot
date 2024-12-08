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
        std::pair<nsgaii::Individual, nsgaii::Individual> crossover(std::pair<nsgaii::Individual, nsgaii::Individual> selected_parents) override;
        std::pair<int, int> sbx(int p1_cycle, int p2_cycle, int cycle_min, int cycle_max, float eta);

        void calucObjectiveFunction(nsgaii::Individual& individual);
        float makespan(std::vector<std::array<float, 4>>& T_span);
        float soc_HiLowTime(std::vector<std::array<float, 2>> T_SOC_HiLow);

        void calcSOCHiLow(nsgaii::Individual& individual);
        float calcChargingTime(float& soc_start, int& soc_target);

        void testTwenty();

        int calcCycleMax(nsgaii::Individual& p1, nsgaii::Individual& p2, int& i);
        float calcLastElapsedTime(nsgaii::Individual& individual, int& i);
        int calcCycle(nsgaii::Individual& individual, int& i);
        float calcTimeChromosome(int& cycle, int& last_return, int& charging_position, float elapsed_time);

        void fixAndPenalty(nsgaii::Individual& individual);
        void individualResize(nsgaii::Individual& individual, int& new_charging_number);
        
    private:
        int min_charge_number;        // 最小充電回数
        std::vector<float> charge_timing;
        float T_cycle;  // 1回のタスクにかかる時間
        float E_cycle;  // 1回のタスクの放電量
    };
} // namespace charge_schedule
