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
        individual.time_chromosome.resize(individual.charging_number);
        individual.soc_chromosome.resize(individual.charging_number);
        individual.T_span.resize(individual.charging_number + 1);
        individual.T_SOC_HiLow.resize(individual.charging_number + 1);
        individual.W.resize(individual.charging_number + 1);
        
        int last_return_position = 0;
        int next_return_position = 0;
        int soc_charging_start = 0;
        float elapsed_time = 0;
        int W_total = 0;
        int soc_zero_cycle = 0;
        int i = 0;
        int max_cycle = static_cast<int>(std::floor((T_max - T_cycle) / T_cycle));

        while (i < individual.charging_number && 0 < std::floor((T_max - T_cycle - elapsed_time) / T_cycle))
        {
            std::uniform_int_distribution<> position_dist(0, 1);
            int charging_timing_position = position_dist(gen);
            if (i == 0){
                soc_zero_cycle = std::floor(100 / E_cycle);
            } else {
                soc_zero_cycle = std::floor((individual.soc_chromosome[i - 1] - E_cs[last_return_position]) / E_cycle);
            }
            max_cycle = std::floor((T_max - T_cycle - elapsed_time) / T_cycle);
            int cycle_limit = std::min(soc_zero_cycle, max_cycle);
            std::uniform_int_distribution<> cycle_dist(0, cycle_limit);
            int cycle = cycle_dist(gen);

            if (last_return_position == charging_timing_position) {
                individual.time_chromosome[i] = cycle * T_cycle + elapsed_time;
                switch (charging_timing_position) {
                    case 0:
                        next_return_position = 1;
                        ++W_total;
                        break;
                    case 1:
                        next_return_position = 0;
                        break;
                    default:
                        break;
                }
            } else {
                individual.time_chromosome[i] = cycle * T_cycle - T_move[charging_timing_position] + elapsed_time;
                switch (charging_timing_position) {
                    case 0:
                        next_return_position = 1;
                        break;
                    case 1:
                        next_return_position = 0;
                        break;
                    default:
                        break;
                }
            }

            W_total += cycle;

            if (i == 0) {
                soc_charging_start = 100 - (((individual.time_chromosome[i] - elapsed_time) / T_cycle) * E_cycle + E_cs[charging_timing_position]);
            } else {
                soc_charging_start = individual.soc_chromosome[i - 1] - (E_cs[last_return_position] + ((individual.time_chromosome[i] - elapsed_time) / T_cycle) * E_cycle + E_cs[charging_timing_position]);
            }

            std::uniform_int_distribution<> target_SOC_dist(soc_charging_start, 100);
            individual.soc_chromosome[i] = target_SOC_dist(gen);
            individual.W[i] = W_total;
            individual.T_span[i][0] = individual.time_chromosome[i] - elapsed_time;
            individual.T_span[i][1] = T_cs[charging_timing_position];
            individual.T_span[i][2] = calcChargingTime(soc_charging_start, individual.soc_chromosome[i]);
            individual.T_span[i][3] = T_cs[next_return_position];

            elapsed_time += individual.T_span[i][0] + individual.T_span[i][1] + individual.T_span[i][2] + individual.T_span[i][3];
            last_return_position = next_return_position;
            ++i;
        }

        if (i < individual.charging_number)
        {
            individual.charging_number = i;
            individual.time_chromosome.resize(individual.charging_number);
            individual.soc_chromosome.resize(individual.charging_number);
            individual.T_span.resize(individual.charging_number + 1);
            individual.T_SOC_HiLow.resize(individual.charging_number + 1);
            individual.W.resize(individual.charging_number + 1);
        }

        if (individual.W[i-1] < W_target) {
            individual.W[i] = W_target;
            switch (last_return_position) {
            case 0:
                individual.T_span[i][0] = ((W_target - individual.W[i-1]) / T_cycle) - T_move[1];
                break;
            case 1:
                individual.T_span[i][0] = (W_target - individual.W[i-1]) / T_cycle;
                break;
            default:
                break;
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
            std::cout << "charging_number: " << combind_population[i].charging_number << std::endl;
            for (int time : combind_population[i].time_chromosome)
            {
                std::cout << time << " ";
            }
            std::cout << std::endl;
            for (int soc : combind_population[i].soc_chromosome)
            {
                std::cout << soc << " ";
            }
            std::cout << std::endl;
            for (int W : combind_population[i].W)
            {
                std::cout << W << " ";
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