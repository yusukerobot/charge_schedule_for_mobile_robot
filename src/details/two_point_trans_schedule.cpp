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
        min_charge_number = W_target / ((100 - E_cs[0]) / E_cycle);
        // testTwenty();
    }

    void TwoTransProblem::testTwenty() {
        int W_total = 0;
        float Time_total = 0.0f;
        std::cout << "----Test Twenty Probrem----" << std::endl;
        std::cout << "T cycle: " << T_cycle << std::endl;
        std::cout << "E cycle: " << E_cycle << std::endl;
        W_total += std::floor(80/E_cycle);
        std::cout << "One span Work Count: " << std::floor(80/E_cycle) << std::endl;
        std::cout << "One span Work Time: " << std::floor(80/E_cycle)* T_cycle << std::endl;
        std::cout << "One span Outward E: " << E_cs[0] << std::endl;
        std::cout << "One span Charging Time: " << (SOC_cccv - (20 - E_cs[0])) / r_cc + (10 / r_cv) << std::endl;
        std::cout << "One span Return E: " << E_cs[1] << std::endl;
        ++W_total;
        std::cout << "Work count ++: "  << std::endl;
        Time_total += std::floor(80/E_cycle)* T_cycle + T_move[0] + (SOC_cccv - (20 - E_cs[0])) / r_cc + (10 / r_cv) + E_cs[1];
        std::cout << "One span total Work count: " << W_total << std::endl;
        std::cout << "One span total Time: " << Time_total << std::endl;
        std::cout << "---" << std::endl;

        W_total += std::floor((100 - E_cs[1] - 20)/E_cycle);
        std::cout << "Second span Work Count: " << std::floor((100 - E_cs[1] - 20)/E_cycle) << std::endl;
        std::cout << "Second span Work Time: " << std::floor((100 - E_cs[1] - 20)/E_cycle)* T_cycle << std::endl;
        std::cout << "Second span Outward E: " << E_cs[1] << std::endl;
        std::cout << "Second span Charging Time: " << (SOC_cccv - (20 - E_cs[1])) / r_cc + (10 / r_cv) << std::endl;
        std::cout << "Second span Return E: " << E_cs[0] << std::endl;
        Time_total += std::floor((100 - E_cs[1] - 20)/E_cycle)* T_cycle + T_move[1] + (SOC_cccv - (20 - E_cs[1])) / r_cc + (10 / r_cv) + E_cs[0];
        std::cout << "Second span total Work count: " << W_total << std::endl;
        std::cout << "Second span total Time: " << Time_total << std::endl;
        std::cout << "---" << std::endl;

        // Time_total += (W_target - W_total) / T_cycle - T_move[1];
        // std::cout << "Total Time of 40 works: " << Time_total << std::endl;
        W_total += (180 - Time_total) / T_cycle;
        std::cout << "Total Works at 180 min: " << W_total << std::endl;

        // W_total += std::floor((100 - E_cs[0] - 20)/E_cycle);
        // std::cout << "Third span Work Count: " << std::floor((100 - E_cs[0] - 20)/E_cycle) << std::endl;
        // std::cout << "Third span Work Time: " << std::floor((100 - E_cs[0] - 20)/E_cycle)* T_cycle << std::endl;
        // std::cout << "Third span Outward E: " << E_cs[0] << std::endl;
        // std::cout << "Third span Charging Time: " << (SOC_cccv - (20 - E_cs[1])) / r_cc + (10 / r_cv) << std::endl;
        // std::cout << "Third span Return E: " << E_cs[1] << std::endl;
        // ++W_total;
        // std::cout << "Work count ++: "  << std::endl;
        // Time_total += std::floor((100 - E_cs[0] - 20)/E_cycle)* T_cycle + T_move[0] + (SOC_cccv - (20 - E_cs[0])) / r_cc + (10 / r_cv) + E_cs[1];
        // std::cout << "Third span total Work count: " << W_total << std::endl;
        // std::cout << "Third span total Time: " << Time_total << std::endl;
        // std::cout << "---" << std::endl;

        // W_total += std::floor((100 - E_cs[1] - 20)/E_cycle);
        // std::cout << "Four span Work Count: " << std::floor((100 - E_cs[1] - 20)/E_cycle) << std::endl;
        // std::cout << "Four span Work Time: " << std::floor((100 - E_cs[1] - 20)/E_cycle)* T_cycle << std::endl;
        // std::cout << "Four span Outward E: " << E_cs[1] << std::endl;
        // std::cout << "Four span Charging Time: " << (SOC_cccv - (20 - E_cs[1])) / r_cc + (10 / r_cv) << std::endl;
        // std::cout << "Four span Return E: " << E_cs[0] << std::endl;
        // Time_total += std::floor((100 - E_cs[1] - 20)/E_cycle)* T_cycle + T_move[1] + (SOC_cccv - (20 - E_cs[1])) / r_cc + (10 / r_cv) + E_cs[0];
        // std::cout << "v span total Work count: " << W_total << std::endl;
        // std::cout << "Four span total Time: " << Time_total << std::endl;
        // std::cout << "---" << std::endl;

        std::cout << "---------------------------" << std::endl;
    }

    nsgaii::Individual TwoTransProblem::generateIndividual()
    {
        nsgaii::Individual individual(max_charge_number);

        std::random_device rd; // ノイズを使用したシード
        std::mt19937 gen(rd()); // メルセンヌ・ツイスタエンジン
        
        std::uniform_int_distribution<> charging_number_dist(min_charge_number, max_charge_number);
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
        int time_limit_cycle = static_cast<int>(std::floor((T_max - T_cycle) / T_cycle));
        int soc_zero_cycle = 0;
        int i = 0;

        while (i < individual.charging_number && 0 < std::floor((T_max - T_cycle - elapsed_time) / T_cycle))
        {
            std::uniform_int_distribution<> position_dist(0, 1);
            int charging_timing_position = position_dist(gen);
            if (i == 0){
                soc_zero_cycle = std::floor(100 / E_cycle);
            } else {
                soc_zero_cycle = std::floor((individual.soc_chromosome[i - 1] - E_cs[last_return_position]) / E_cycle);
            }
            time_limit_cycle = std::floor((T_max - T_cycle - elapsed_time) / T_cycle);
            int cycle_limit = std::min(soc_zero_cycle, time_limit_cycle);
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

            if (W_total >= W_target) {
                // std::cout <<"初期充電回数を満たす前に目標タスク完了" << std::endl;
                break;
            }

            if (i == 0) {
                soc_charging_start = 100 - (((individual.time_chromosome[i] - elapsed_time) / T_cycle) * E_cycle + E_cs[charging_timing_position]);
            } else {
                soc_charging_start = individual.soc_chromosome[i - 1] - (E_cs[last_return_position] + ((individual.time_chromosome[i] - elapsed_time) / T_cycle) * E_cycle + E_cs[charging_timing_position]);
            }

            std::uniform_int_distribution<> target_SOC_dist(soc_charging_start, 100);
            individual.soc_chromosome[i] = target_SOC_dist(gen);
            individual.W[i] = W_total;
            individual.E_return[i] = E_cs[next_return_position];
            individual.soc_charging_start[i] = soc_charging_start;
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
            individual.E_return.resize(individual.charging_number + 1);
            individual.soc_charging_start.resize(individual.charging_number + 1);
            individual.W.resize(individual.charging_number + 1);
            if (W_total >= W_target) {
                individual.W[i] = W_target;
                individual.T_span[i][0] = (individual.W[i] - individual.W[i-1]) * T_cycle;
            }
            if (W_total < W_target && 0 < std::floor((T_max - T_cycle - elapsed_time) / T_cycle)) {
                individual.W[i] = W_target;
                individual.T_span[i][0] = (individual.W[i] - individual.W[i-1]) * T_cycle;
                ++individual.penalty;
            }
        } else {
            if (std::floor((individual.soc_chromosome[i - 1] - E_cs[last_return_position]) / E_cycle) > (W_target - W_total)) {
                individual.W[i] = W_target;
                individual.T_span[i][0] = (individual.W[i] - individual.W[i-1]) * T_cycle;
                elapsed_time += individual.T_span[i][0];
                if (elapsed_time > T_max) {
                    ++individual.penalty;
                }
            } else {
                individual.W[i] = W_target;
                individual.T_span[i][0] = (individual.W[i] - individual.W[i-1]) * T_cycle;
                ++individual.penalty;
            }
        }

        return individual;
    }

    void TwoTransProblem::generateFirstCombinedPopulation()
    {
        for (size_t i = 0; i < 2*population_size; ++i)
        {
            combind_population[i] = generateIndividual();
            // std::cout << "---" << std::endl;
            // std::cout << "individual[ " << i << " ]" << std::endl;
            // std::cout << "charging_number: " << combind_population[i].charging_number << std::endl;
            // for (int time : combind_population[i].time_chromosome)
            // {
            //     std::cout << time << " ";
            // }
            // std::cout << std::endl;
            // for (int soc : combind_population[i].soc_chromosome)
            // {
            //     std::cout << soc << " ";
            // }
            // std::cout << std::endl;
            // for (int W : combind_population[i].W)
            // {
            //     std::cout << W << " ";
            // }
            // std::cout << std::endl;
            // std::cout << "T_span[0] : ";
            // for (auto& span : combind_population[i].T_span)
            // {
            //     std::cout << span[0] << " ";
            // }
            // std::cout << std::endl;
            // std::cout << "T_span[1] : ";
            // for (auto& span : combind_population[i].T_span)
            // {
            //     std::cout << span[1] << " ";
            // }
            // std::cout << std::endl;
            // std::cout << "T_span[2] : ";
            // for (auto& span : combind_population[i].T_span)
            // {
            //     std::cout << span[2] << " ";
            // }
            // std::cout << std::endl;
            // std::cout << "T_span[3] : ";
            // for (auto& span : combind_population[i].T_span)
            // {
            //     std::cout << span[3] << " ";
            // }
            // std::cout << std::endl;
            // float total_time = 0.0f;
            // for (auto& span : combind_population[i].T_span)
            // {
            //     for (auto& section : span)
            //     total_time += section;
            // }
            // std::cout << "Total Time of " << W_target << " works: " << total_time << std::endl;
            // std::cout << "penalty count: " << combind_population[i].penalty << std::endl;
            // std::cout << "---" << std::endl;
        }
    }

    void TwoTransProblem::evaluatePopulation(std::vector<nsgaii::Individual>& population)
    {
        for (nsgaii::Individual& individual : population)
        {
            calucObjectiveFunction(individual);
            // std::cout << "f1: " << individual.f1 << std::endl;
            // std::cout << "f2: " << individual.f2 << std::endl;
            // std::cout << "penalty count: " << individual.penalty << std::endl;
            // std::cout << "---" << std::endl;
        }
    }

    void TwoTransProblem::calucObjectiveFunction(nsgaii::Individual& individual)
    {
        calcSOCHiLow(individual);
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

    void TwoTransProblem::calcSOCHiLow(nsgaii::Individual& individual) {
        std::array<float, 3> T_socHi = {};
        std::array<float, 3> T_socLow = {};
        float last_soc_t = 0.0f;
        for (int i = 0; i < individual.charging_number+1; ++i) {
            if (i == 0) {
                last_soc_t = 100;
            } else {
                last_soc_t = individual.soc_chromosome[i - 1];
            }

            if (individual.soc_charging_start[i] <= SOC_Hi && SOC_Hi <= last_soc_t) {
                T_socHi[0] = ((last_soc_t - SOC_Hi) / (last_soc_t - individual.soc_charging_start[i])) * (individual.T_span[i][0] + individual.T_span[i][1]);
            } else {T_socHi[0] = 0;}
            if (individual.soc_charging_start[i] <= SOC_Hi && SOC_Hi <= individual.soc_chromosome[i]) {
                T_socHi[1] = ((individual.soc_chromosome[i] - SOC_Hi) / (individual.soc_chromosome[i] - individual.soc_charging_start[i])) * individual.T_span[i][2];
            } else {T_socHi[1] = 0;}
            if (individual.soc_chromosome[i] - individual.E_return[i] <= SOC_Hi && SOC_Hi <= individual.soc_chromosome[i]) {
                T_socHi[2] = ((individual.soc_chromosome[i] - SOC_Hi) / (individual.soc_chromosome[i] - (individual.soc_chromosome[i] - individual.E_return[i]))) * individual.T_span[i][3];
            } else {T_socHi[2] = 0;}
            if (individual.soc_charging_start[i] <= SOC_Low && SOC_Low <= last_soc_t) {
                T_socLow[0] = ((SOC_Low - individual.soc_charging_start[i]) / (last_soc_t - individual.soc_charging_start[i])) * (individual.T_span[i][0] + individual.T_span[i][1]);
            } else {T_socHi[0] = 0;}
            if (individual.soc_charging_start[i] <= SOC_Low && SOC_Low <= individual.soc_chromosome[i]) {
                T_socLow[1] = ((SOC_Low - individual.soc_charging_start[i]) / (individual.soc_chromosome[i] - individual.soc_charging_start[i])) * individual.T_span[i][2];
            } else {T_socHi[1] = 0;}
            if (individual.soc_chromosome[i] - individual.E_return[i] <= SOC_Low && SOC_Low <= individual.soc_chromosome[i]) {
                T_socLow[2] = ((SOC_Low - (individual.soc_chromosome[i] - individual.E_return[i])) / (individual.soc_chromosome[i] - (individual.soc_chromosome[i] - individual.E_return[i]))) * individual.T_span[i][3];
            } else {T_socHi[2] = 0;}

            for (int j = 0; j < 3; ++j) {
                individual.T_SOC_HiLow[i][0] += T_socHi[j];
                individual.T_SOC_HiLow[i][1] += T_socLow[j];
            }
        }
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