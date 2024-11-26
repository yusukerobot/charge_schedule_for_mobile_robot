#include <random>
#include <cmath>
#include <iostream>
#include "two_point_trans_schedule.hpp"

namespace charge_schedule
{
    TwoTransProblem::TwoTransProblem(const std::string& config_file_path)
    : nsgaii::ScheduleNsgaii(config_file_path)
    {
        for (size_t i = 0; i < visited_number; ++i)
        {
            T_cycle += T_move[i] + T_standby[i];
            E_cycle += E_move[i] + E_standby[i];
        }
    }

    nsgaii::Individual TwoTransProblem::generateIndividual()
    {
        nsgaii::Individual individual(max_charge_number);

        std::random_device rd; // ノイズを使用したシード
        std::mt19937 gen(rd()); // メルセンヌ・ツイスタエンジン
        
        std::uniform_int_distribution<> charging_number_dist(0, max_charge_number);
        individual.charging_number = charging_number_dist(gen);
        
        int return_position = 0;
        int soc_charging_start = 0;
        float elapsed_time = 0;
        int W_total = 0;
        size_t i = 0;

        while (i < individual.charging_number && elapsed_time < T_max && W_total < W_target)
        {
            std::uniform_int_distribution<> cycle_dist(0, static_cast<int>(std::floor(((T_max - elapsed_time) / T_cycle) - 1)));
            std::uniform_int_distribution<> position_dist(0, visited_number - 1);
            int charging_position = position_dist(gen);
            int cycle = cycle_dist(gen);
            if (i == 0)
            {
                switch (charging_position)
                {
                    case 0:
                        individual.time_chromosome[i] = cycle * T_cycle;
                        return_position = 1;
                        break;
                    case 1:
                        individual.time_chromosome[i] = cycle * T_cycle + (T_cycle - T_move[visited_number - 1]);
                        return_position = 0;
                        break;
                    default:
                        break;
                }
                W_total += cycle;
                soc_charging_start = 100 - ((individual.time_chromosome[i] - elapsed_time) / T_cycle * E_cycle + E_cs[charging_position]);
            } else{
                switch (charging_position)
                {
                    case 0:
                        individual.time_chromosome[i] = elapsed_time + cycle * T_cycle;
                        soc_charging_start = individual.soc_chromosome[i - 1] - (E_cs[return_position] + (individual.time_chromosome[i] - elapsed_time) / T_cycle * E_cycle + E_cs[charging_position]);
                        return_position = 1;
                        break;
                    case 1:
                        individual.time_chromosome[i] = elapsed_time + (cycle * T_cycle) + (T_cycle - T_move[visited_number - 1]);
                        soc_charging_start = individual.soc_chromosome[i - 1] - (E_cs[return_position] + (individual.time_chromosome[i] - elapsed_time) / T_cycle * E_cycle + E_cs[charging_position]);
                        return_position = 0;
                        break;
                    default:
                        break;
                }
            }
            std::uniform_int_distribution<> target_SOC_dist(soc_charging_start, 100);
            individual.soc_chromosome[i] = target_SOC_dist(gen);
            individual.W[i] = W_total;
            individual.T_span[i][0] = individual.time_chromosome[i];
            individual.T_span[i][1] = T_cs[charging_position];
            individual.T_span[i][2] = calcChargingTime(soc_charging_start, individual.soc_chromosome[i]);
            individual.T_span[i][3] = T_cs[return_position];
            elapsed_time += individual.T_span[i][0] + individual.T_span[i][1] + individual.T_span[i][2] + individual.T_span[i][3];
            ++i;
        }
        if (individual.charging_number < max_charge_number)
        {
            for (size_t i = individual.charging_number; i < max_charge_number; ++i)
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

    float TwoTransProblem::calcChargingTime(int& soc_charging_start, int& soc_target)
    {
        float charging_time = 0;

        if (SOC_cccv < soc_charging_start){
            charging_time = (soc_target - soc_charging_start) / r_cv;
        } else if (soc_charging_start < SOC_cccv && SOC_cccv < soc_target){
            charging_time = (SOC_cccv - soc_charging_start) / r_cc + (soc_target - SOC_cccv) / r_cv;
        } else{
            charging_time = (soc_target - soc_charging_start) / r_cc;
        }

        return charging_time;
    }
}