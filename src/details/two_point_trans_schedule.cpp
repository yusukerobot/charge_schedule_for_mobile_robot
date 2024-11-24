#include "two_point_trans_schedule.hpp"

namespace charge_schedule
{
    TowTransProblem::TowTransProblem(const std::string& config_file_path)
    : nsgaii::ScheduleNsgaii(config_file_path)
    {}

    void TowTransProblem::generateFirstCombinedPopulation()
    {

    }

    void TowTransProblem::evaluatePopulation(std::vector<nsgaii::Individual>& population)
    {
        for (nsgaii::Individual& individual : population)
        {
            calucObjectiveFunction(individual);
        }
    }

    void TowTransProblem::calucObjectiveFunction(nsgaii::Individual& individual)
    {
        std::vector<float> T_span;
        std::vector<float> T_socHi;
        std::vector<float> T_socLow;

        individual.f1 = makespan(T_span);
        individual.f2 = soc_HiLowTime(T_socHi, T_socLow)
    }

    float TowTransProblem::makespan(std::vector<float>& T_span)
    {
        float makespan = 0;
        for (float& time : T_span)
        {
            makespan += time;
        }
        return makespan;
    }

    float TowTransProblem::soc_HiLowTime(std::vector<float>& T_socHi, std::vector<float>& T_socLow)
    {
        float time = 0;
        for (size_t i = 0; i < T_socHi.size(); ++i)
        {
            time += T_socHi[i] + T_socLow[i];
        }
        return time;
    }
}