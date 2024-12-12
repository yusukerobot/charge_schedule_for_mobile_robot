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

        void generateFirstParents() override;
        void generateChildren() override;
        void evaluatePopulation(std::vector<nsgaii::Individual>& population) override;
        std::pair<nsgaii::Individual, nsgaii::Individual> crossover(std::pair<nsgaii::Individual, nsgaii::Individual> selected_parents) override;
        std::pair<nsgaii::Individual, nsgaii::Individual> second_crossover(std::pair<nsgaii::Individual, nsgaii::Individual> selected_parents);
        std::pair<int, int> int_sbx(int& p1, int& p2, std::pair<int, int>& gene_min, std::pair<int, int>& gene_max, float eta);
        std::pair<float, float> float_sbx(float& p1, float& p2, std::pair<float, float>& gene_min, std::pair<float, float>& gene_max, float eta);

        void calucObjectiveFunction(nsgaii::Individual& individual);
        float makespan(std::vector<std::array<float, 4>>& T_span);
        float soc_HiLowTime(std::vector<float> T_SOC_HiLow);

        void calcSOCHiLow(nsgaii::Individual& individual);
        float calcChargingTime(float& soc_start, int& soc_target);

        void testTwenty();

        int calcCycleMax(nsgaii::Individual& individual, int charging_position, int& last_return_position, int& i);
        float calcElapsedTime(nsgaii::Individual& individual, int& i);
        float calcTimeChromosome(int& cycle, int& last_return, int& charging_position, float elapsed_time);
        float calcSOCchargingStart(float first_soc, int& cycle, int& last_return, int& charging_position);
        int calcTotalWork(int& cycle, int& last_return, int& charging_position);
        std::pair<int, int> timeToCycleAndPosition(float& target_time, int& last_return_position, float& elapsed_time);

        void fixAndPenalty(nsgaii::Individual& individual);
        void additionalGen(nsgaii::Individual& individual);
        void individualResize(nsgaii::Individual& individual, int& new_charging_number);
        
    private:
        int min_charge_number;        // 最小充電回数
        int soc_minimum;              // soc最小値
        std::vector<float> T_timing;
        std::vector<float> E_timing;
        float T_cycle;  // 1回のタスクにかかる時間
        float E_cycle;  // 1回のタスクの放電量
    };
} // namespace charge_schedule
