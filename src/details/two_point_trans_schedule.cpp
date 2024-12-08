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
        min_charge_number = std::floor((W_target * E_cycle - E_cs[0]) / 100);
        // std::cout << "総放電量: " << W_target * E_cycle - E_cs[0] << std::endl;
        // std::cout << "min_charge_number: " << min_charge_number << std::endl;
        charge_timing.resize(4, 0);
        charge_timing[0] = T_standby[0];                              // last: 0, charge: 0, 荷積み
        charge_timing[1] = T_standby[1];                              // last: 1, charge: 1,荷降ろし
        charge_timing[2] = T_standby[0] + T_move[0] + T_standby[1];   // last: 0, charge: 1, 
        charge_timing[3] = T_standby[1] + T_move[1] + T_standby[0];   // last: 1, charge: 0
        // testTwenty();
    }

    nsgaii::Individual TwoTransProblem::generateIndividual()
    {
        std::random_device rd; // ノイズを使用したシード
        std::mt19937 gen(rd()); // メルセンヌ・ツイスタエンジン
        std::uniform_int_distribution<> charging_number_dist(min_charge_number, max_charge_number);
        
        nsgaii::Individual individual(charging_number_dist(gen));
        
        int last_return_position = 0;
        float elapsed_time = 0;
        int W_total = 0;
        int i = 0;

        while (i < individual.charging_number)
        {
            // charging_positionの決定
            std::uniform_int_distribution<> position_dist(0, 1);
            int charging_timing_position = position_dist(gen);
            int return_position = (charging_timing_position == 0) ? 1 : 0;
            // cycleの決定
            int soc_zero_cycle = (i == 0) ? std::floor(100 / E_cycle) : std::floor((individual.soc_chromosome[i - 1] - E_cs[last_return_position]) / E_cycle);
            std::uniform_int_distribution<> cycle_dist(0, soc_zero_cycle);
            int cycle = cycle_dist(gen);

            individual.cycle[i] = cycle;
            W_total += (last_return_position == charging_timing_position == 1) ? cycle : cycle + 1;

            individual.time_chromosome[i] = calcTimeChromosome(cycle, last_return_position, charging_timing_position, elapsed_time);

            if (i == 0) {
                individual.soc_charging_start[i] = 100 - (((individual.time_chromosome[i] - elapsed_time) / T_cycle) * E_cycle + E_cs[charging_timing_position]);
            } else {
                individual.soc_charging_start[i] = individual.soc_chromosome[i - 1] - (E_cs[last_return_position] + ((individual.time_chromosome[i] - elapsed_time) / T_cycle) * E_cycle + E_cs[charging_timing_position]);
            }

            std::uniform_int_distribution<> target_SOC_dist(individual.soc_charging_start[i], 100);
            individual.soc_chromosome[i] = target_SOC_dist(gen);

            individual.T_span[i][0] = individual.time_chromosome[i] - elapsed_time;
            individual.T_span[i][1] = T_cs[charging_timing_position];
            individual.T_span[i][2] = calcChargingTime(individual.soc_charging_start[i], individual.soc_chromosome[i]);
            individual.T_span[i][3] = T_cs[return_position];
            individual.W[i] = W_total;
            individual.E_return[i] = E_cs[return_position];
            individual.charging_position[i] = charging_timing_position;
            individual.return_position[i] = return_position;

            elapsed_time += individual.T_span[i][0] + individual.T_span[i][1] + individual.T_span[i][2] + individual.T_span[i][3];
            last_return_position = return_position;
            ++i;
        }

        // fixAndPenalty(individual);
        return individual;
    }

    void TwoTransProblem::generateFirstCombinedPopulation()
    {
        for (int i = 0; i < combind_population.size(); ++i)
        {
            combind_population[i] = generateIndividual();
            // if (combind_population[i].penalty > 0) --i;
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

    std::pair<nsgaii::Individual, nsgaii::Individual> TwoTransProblem::crossover(std::pair<nsgaii::Individual, nsgaii::Individual> selected_parents) {
        std::pair<nsgaii::Individual, nsgaii::Individual> child = std::make_pair(nsgaii::Individual(selected_parents.first.charging_number), nsgaii::Individual(selected_parents.second.charging_number));

        int i = 0;
        int last_return_position = 0;
        float c1_elapsed_time = 0;
        float c2_elapsed_time = 0;
        int c1_W_total = 0;
        int c2_W_total = 0;

        while (i < child.first.charging_number) {
            std::random_device rd; // ノイズを使用したシード
            std::mt19937 gen(rd()); // メルセンヌ・ツイスタエンジン
            int cycle_max = calcCycleMax(selected_parents.first, selected_parents.second, i); 
            int small_gen_cycle = std::min(calcCycle(selected_parents.first, i), calcCycle(selected_parents.second, i));
            int big_gen_cycle = std::max(calcCycle(selected_parents.first, i), calcCycle(selected_parents.second, i));
            int cycle_min = small_gen_cycle - std::abs(cycle_max - big_gen_cycle);

            std::pair<int, int> generated_cycle = sbx(calcCycle(selected_parents.first, i), calcCycle(selected_parents.second, i), cycle_min, cycle_max, 0.5);
            std::uniform_int_distribution<> timing_dist(0, 1);
            int charge_timing_position = timing_dist(gen);
            int return_position = (charge_timing_position == 0) ? 1 : 0;

            child.first.cycle[i] = generated_cycle.first;
            c1_W_total += (last_return_position == charge_timing_position == 1) ? generated_cycle.first : generated_cycle.first + 1;
            child.second.cycle[i] = generated_cycle.second;
            c2_W_total += (last_return_position == charge_timing_position == 1) ? generated_cycle.second : generated_cycle.second + 1;

            child.first.time_chromosome[i] = calcTimeChromosome(generated_cycle.first, last_return_position, charge_timing_position, calcLastElapsedTime(selected_parents.first, i));
            child.second.time_chromosome[i] = calcTimeChromosome(generated_cycle.second, last_return_position, charge_timing_position, calcLastElapsedTime(selected_parents.second, i));
            
            if (i == 0) {
                child.first.soc_charging_start[i] = 100 - (((child.first.time_chromosome[i] - c1_elapsed_time) / T_cycle) * E_cycle + E_cs[charge_timing_position]);
                child.second.soc_charging_start[i] = 100 - (((child.second.time_chromosome[i] - c2_elapsed_time) / T_cycle) * E_cycle + E_cs[charge_timing_position]);
            } else {
                child.first.soc_charging_start[i] = child.first.soc_chromosome[i - 1] - (E_cs[last_return_position] + ((child.first.time_chromosome[i] - c1_elapsed_time) / T_cycle) * E_cycle + E_cs[charge_timing_position]);
                child.second.soc_charging_start[i] = child.second.soc_chromosome[i - 1] - (E_cs[last_return_position] + ((child.second.time_chromosome[i] - c2_elapsed_time) / T_cycle) * E_cycle + E_cs[charge_timing_position]);
            }

            std::uniform_int_distribution<> c1_target_SOC_dist(child.first.soc_charging_start[i], 100);
            std::uniform_int_distribution<> c2_target_SOC_dist(child.second.soc_charging_start[i], 100);
            child.first.soc_chromosome[i] = c1_target_SOC_dist(gen);
            child.second.soc_chromosome[i] = c2_target_SOC_dist(gen);

            child.first.T_span[i][0] = child.first.time_chromosome[i] - c1_elapsed_time;
            child.first.T_span[i][1] = T_move[charge_timing_position];
            child.first.T_span[i][2] = calcChargingTime(child.first.soc_charging_start[i], child.first.soc_chromosome[i]);
            child.first.T_span[i][3] = T_move[return_position];

            child.first.W[i] = c1_W_total;
            child.first.E_return[i] = E_cs[return_position];
            child.first.charging_position[i] = charge_timing_position;
            child.first.return_position[i] = return_position;


            child.second.T_span[i][0] = child.second.time_chromosome[i] - c2_elapsed_time;
            child.second.T_span[i][1] = T_move[charge_timing_position];
            child.second.T_span[i][2] = calcChargingTime(child.second.soc_charging_start[i], child.second.soc_chromosome[i]);
            child.second.T_span[i][3] = T_move[return_position];

            child.second.W[i] = c2_W_total;
            child.second.E_return[i] = E_cs[return_position];
            child.second.charging_position[i] = charge_timing_position;
            child.second.return_position[i] = return_position;

            c1_elapsed_time += child.first.T_span[i][0] + child.first.T_span[i][1] + child.first.T_span[i][2] + child.first.T_span[i][3];
            c2_elapsed_time += child.second.T_span[i][0] + child.second.T_span[i][1] + child.second.T_span[i][2] + child.second.T_span[i][3];
            last_return_position = return_position;
            ++i;
        }
        while (i < child.second.charging_number) {
            std::random_device rd; // ノイズを使用したシード
            std::mt19937 gen(rd()); // メルセンヌ・ツイスタエンジン
            std::uniform_int_distribution<> timing_dist(0, 1);
            int charge_timing_position = timing_dist(gen);
            int return_position = (charge_timing_position == 0) ? 1 : 0;
            int cycle = std::floor((child.second.soc_chromosome[i - 1] - E_cs[last_return_position]) / E_cycle);

            child.second.time_chromosome[i] = calcTimeChromosome(cycle, last_return_position, charge_timing_position, c2_elapsed_time);
            child.second.soc_charging_start[i] = child.second.soc_chromosome[i - 1] - (E_cs[last_return_position] + ((child.second.time_chromosome[i] - c2_elapsed_time) / T_cycle) * E_cycle + E_cs[charge_timing_position]);
            std::uniform_int_distribution<> c2_target_SOC_dist(child.second.soc_charging_start[i], 100);
            child.second.soc_chromosome[i] = c2_target_SOC_dist(gen);

            child.second.T_span[i][0] = child.second.time_chromosome[i] - c2_elapsed_time;
            child.second.T_span[i][1] = T_move[charge_timing_position];
            child.second.T_span[i][2] = calcChargingTime(child.second.soc_charging_start[i], child.second.soc_chromosome[i]);
            child.second.T_span[i][3] = T_move[return_position];

            child.second.W[i] = c2_W_total;
            child.second.E_return[i] = E_cs[return_position];
            child.second.charging_position[i] = charge_timing_position;
            child.second.return_position[i] = return_position;
            
            c2_elapsed_time += child.second.T_span[i][0] + child.second.T_span[i][1] + child.second.T_span[i][2] + child.second.T_span[i][3];
            last_return_position = return_position;
            ++i;
        }

        fixAndPenalty(child.first);
        fixAndPenalty(child.second);
        return child;
    }

    float TwoTransProblem::calcTimeChromosome(int& cycle, int& last_return, int& charging_position, float elapsed_time) {
        float time = 0;
        if (last_return == charging_position) {
            time = cycle * T_cycle + charge_timing[charging_position] + elapsed_time;
        } else {
            time = (charging_position == 0) ? cycle * T_cycle + charge_timing[2] + elapsed_time : cycle * T_cycle + charge_timing[3] + elapsed_time;
        }
        return time;
    }

    int TwoTransProblem::calcCycleMax(nsgaii::Individual& p1, nsgaii::Individual& p2, int& i) {
        int cycle = std::floor(100 / E_cycle);
        if (i != 0) {
            cycle = calcCycle((p1.time_chromosome[i] >= p2.time_chromosome[i]) ? p1 : p2, i);
        }

        return cycle;
    }

    float TwoTransProblem::calcLastElapsedTime(nsgaii::Individual& individual, int& i) {
        float elapsed_time = 0;
        if (i != 0) {
            for (int j = 0; j < i; ++j) {
                for (int k = 0; k < 4; ++k) {
                    elapsed_time += individual.T_span[j][k];
                }
            }
        }
        return elapsed_time;
    }

    int TwoTransProblem::calcCycle(nsgaii::Individual& individual, int& i) {
        int cycle = 0;
        cycle = (individual.time_chromosome[i] - calcLastElapsedTime(individual, i)) / T_cycle;
        return cycle;
    }

    std::pair<int, int> TwoTransProblem::sbx(int p1_cycle, int p2_cycle, int cycle_min, int cycle_max, float eta) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dist(0.0, 1.0);

        float u = dist(gen);
        float beta = (u <= 0.5) 
                    ? pow(2 * u, 1.0 / (eta + 1)) 
                    : pow(1.0 / (2 * (1 - u)), 1.0 / (eta + 1));

        int c1_cycle = std::floor(0.5 * ((1 + beta) * p1_cycle + (1 - beta) * p2_cycle));
        int c2_cycle = std::floor(0.5 * ((1 - beta) * p1_cycle + (1 + beta) * p2_cycle));

        // 範囲を制限
        c1_cycle = std::floor(std::max(cycle_min, std::min(c1_cycle, cycle_max)));
        c2_cycle = std::floor(std::max(cycle_min, std::min(c2_cycle, cycle_max)));

        return std::make_pair(c1_cycle, c2_cycle);
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

    float TwoTransProblem::calcChargingTime(float& soc_charging_start, int& soc_target)
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

    void TwoTransProblem::fixAndPenalty(nsgaii::Individual& individual) {
        if (individual.W[individual.charging_number - 1] < W_target) {
            individual.W[individual.charging_number] = W_target;
            float final_discharge = 0;
            if (individual.return_position[individual.charging_number - 1] == 0) {
                individual.T_span[individual.charging_number][0] = (individual.W[individual.charging_number] - individual.W[individual.charging_number - 1]) * T_cycle - T_move[1];
                final_discharge = (individual.W[individual.charging_number] - individual.W[individual.charging_number - 1]) * E_cycle - E_move[1];
            } else {
                individual.T_span[individual.charging_number][0] = (individual.W[individual.charging_number] - individual.W[individual.charging_number - 1]) * T_cycle;
                final_discharge = (individual.W[individual.charging_number] - individual.W[individual.charging_number - 1]) * E_cycle;
            }
            int span_size = individual.T_span.size();
            if (calcLastElapsedTime(individual, span_size) > T_max) {
                ++individual.penalty;
            }
            if (individual.soc_chromosome[individual.charging_number - 1] - final_discharge < 0) {
                ++individual.penalty;
            }
        } else {
            int new_charging_number = 0;
            for (new_charging_number < individual.charging_number; ++new_charging_number;) {
                if (individual.W[new_charging_number] > W_target) break;
            }
            individualResize(individual, new_charging_number);
            individual.T_span[new_charging_number][0] = 0;
            individual.W[new_charging_number] = 0;
            fixAndPenalty(individual);
        }
    }

    void TwoTransProblem::individualResize(nsgaii::Individual& individual, int& new_charging_number) {
        individual.charging_number = new_charging_number;
        individual.time_chromosome.resize(new_charging_number);
        individual.soc_chromosome.resize(new_charging_number);
        individual.T_span.resize(new_charging_number + 1);
        individual.T_SOC_HiLow.resize(new_charging_number + 1);
        individual.E_return.resize(new_charging_number + 1);
        individual.cycle.resize(new_charging_number);
        individual.soc_charging_start.resize(new_charging_number);
        individual.W.resize(new_charging_number + 1);
        individual.charging_position.resize(new_charging_number);
        individual.return_position.resize(new_charging_number);
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
}