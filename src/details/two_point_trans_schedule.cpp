#include <random>
#include <cmath>
#include <iostream>
#include "two_point_trans_schedule.hpp"

namespace charge_schedule
{
    TwoTransProblem::TwoTransProblem(const std::string& config_file_path)
    : nsgaii::ScheduleNsgaii(config_file_path), W_total(0), W_int(0), W_dec(0.0f), T_cycle(0), E_cycle(0)
    {
        for (size_t i = 0; i < visited_number; ++i)
        {
            T_cycle += T_move[i] + T_standby[i];
            E_cycle += E_move[i] + E_standby[i];
        }
    }

    nsgaii::Individual TwoTransProblem::generateIndividual()
    {
        std::random_device rd; // ノイズを使用したシード
        std::mt19937 gen(rd()); // メルセンヌ・ツイスタエンジン

        std::uniform_int_distribution<> charging_number_dist(0, max_charge_number);

        nsgaii::Individual individual(max_charge_number);
        individual.charging_number = charging_number_dist(gen);
        int last_position = 0;
        float W = 0;
        int SOC_start = 0;

        for (size_t i = 0; i < charging_number; ++i)
        {
            std::uniform_int_distribution<> timing_position_dist(0, visited_number - 1);
            int position = timing_position_dist(gen);
            if (i == 0)
            {
                std::uniform_int_distribution<> timing_dist(0, static_cast<int>(std::floor((T_max / T_cycle) - 1)));
                switch (position)
                {
                    case 0:{
                        individual.time_chromosome[i] = timing_dist(gen)* T_cycle + (T_cycle - T_move[visited_number - 1]);
                        W = individual.time_chromosome[i] / T_cycle;
                        SOC_start = 100 - ((W * E_cycle) + E_cs[position]);
                        std::uniform_int_distribution<> target_SOC_dist(SOC_start, 100);
                        individual.soc_chromosome[i] = target_SOC_dist(gen);
                        break;
                    }
                    case 1:{
                        individual.time_chromosome[i] = timing_dist(gen) * T_cycle;
                        W = individual.time_chromosome[i] / T_cycle;
                        SOC_start = 100 - ((W * E_cycle) + E_cs[position]);
                        std::uniform_int_distribution<> target_SOC_dist(SOC_start, 100);
                        individual.soc_chromosome[i] = target_SOC_dist(gen);
                        break;
                    }
                    default:
                        break;
                }
            } else{
                std::uniform_int_distribution<> timing_dist(0, static_cast<int>(std::floor((T_max / T_cycle) - 1)));
                switch (position)
                {
                    case 0:{
                        individual.time_chromosome[i] = timing_dist(gen) * T_cycle;
                        W = individual.time_chromosome[i] / T_cycle;
                        if ( i == 0)
                        {
                            SOC_start = 100 - ((W * E_cycle) + E_cs[position]);
                        } else{
                            SOC_start = individual.soc_chromosome[i - 1] - (E_cs[last_position] + (W * E_cycle) + E_cs[position]);
                        }
                        std::uniform_int_distribution<> target_SOC_dist(SOC_start, 100);
                        individual.soc_chromosome[i] = target_SOC_dist(gen);
                        break;
                    }
                    case 1:{
                        individual.time_chromosome[i] = timing_dist(gen) * T_cycle;
                        W = individual.time_chromosome[i] / T_cycle;
                        if ( i == 0)
                        {
                            SOC_start = 100 - ((W * E_cycle) + E_cs[position]);
                        } else{
                            SOC_start = individual.soc_chromosome[i - 1] - (E_cs[last_position] + (W * E_cycle) + E_cs[position]);
                        }
                        std::uniform_int_distribution<> target_SOC_dist(SOC_start, 100);
                        individual.soc_chromosome[i] = target_SOC_dist(gen);
                        break;
                    }
                    default:
                        break;
                }
            }
            last_position = position;
        }
        if (charging_number < max_charge_number)
        {
            for (size_t i = charging_number; i < max_charge_number; ++i)
            {
                individual.time_chromosome[i] = -1;
                individual.soc_chromosome[i] = -1;
            }
        }

        return individual;
    }

    void TwoTransProblem::generateFirstCombinedPopulation()
    {
        for (size_t i = 0; i < 2*population_size; ++i)
        {
            combind_population[i] = generateIndividual();
            std::cout << "individual[ " << i << " ]" << std::endl;
            for (int time : combind_population[i].time_chromosome)
            {
                std::cout << time << " ";
            }
            std::cout << std::endl;
        }
    }

    void TwoTransProblem::evaluatePopulation(std::vector<nsgaii::Individual>& population)
    {
        for (nsgaii::Individual& individual : population)
        {
            calucObjectiveFunction(individual);
        }
    }

    void TwoTransProblem::calucObjectiveFunction(nsgaii::Individual& individual)
    {
        individual.f1 = makespan(individual.T_span);
        individual.f2 = soc_HiLowTime(individual.T_SOC_HiLow);
    }

    float TwoTransProblem::makespan(std::vector<std::array<float, 4>>& T_span)
    {
        float makespan = 0;
        for (auto& span : T_span)
        {
            for (auto& time : span)
            {
                makespan += time;
            }
        }
        return makespan;
    }

    float TwoTransProblem::soc_HiLowTime(std::vector<std::array<float, 2>> T_SOC_HiLow)
    {
        float hi_low_time = 0;
        for (auto& span : T_SOC_HiLow)
        {
            for (auto& time : span)
            {
                hi_low_time += time;
            }
        }
        return hi_low_time;
    }
}