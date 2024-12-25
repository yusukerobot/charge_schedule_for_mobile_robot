#include <random>
#include <memory>
#include <vector>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <set>
#include <utility>  
#include "two_point_trans_schedule.hpp"

namespace charge_schedule
{
    TwoTransProblem::TwoTransProblem(const std::string& config_file_path)
    : nsgaii::ScheduleNsgaii(config_file_path), soc_minimum(5)
    {
        for (size_t i = 0; i < visited_number; ++i)
        {
            T_cycle += T_move[i] + T_standby[i]; // 3.0
            E_cycle += E_move[i] + E_standby[i]; // 4.9
        }
        float W_total = W_target * E_cycle - E_cs[0]; // 総放電量
        min_charge_number = (W_total > 0) ? std::floor(W_total / 100) : 0;

        T_timing.resize(4, 0);
        T_timing[0] = T_move[1] + T_standby[1] + T_move[0];     // last: 0, charge: 0, 2 min
        T_timing[1] = 0;                                        // last: 1, charge: 1, 0 min
        T_timing[2] = T_standby[1] + T_move[0];                 // last: 1, charge: 0, 1.5 min
        T_timing[3] = T_move[1];                                // last: 0, charge: 1, 1.0 min
        E_timing.resize(4, 0);
        E_timing[0] = E_move[1] + E_standby[1] + E_move[0];      // 3.32             
        E_timing[1] = 0;                                         // 0
        E_timing[2] = E_standby[1] + E_move[0];                  // 2.49
        E_timing[3] = E_move[1];                                 // 0.83
        // testTwenty();
    }

    nsgaii::Individual TwoTransProblem::generateIndividual(const bool& charging_number_random, const int& fixed_charging_number)
    {
        std::random_device rd; // ノイズを使用したシード
        std::mt19937 gen(rd()); // メルセンヌ・ツイスタエンジン

        std::uniform_int_distribution<> charging_number_dist(min_charge_number, max_charge_number);

        nsgaii::Individual individual(max_charge_number);

        if (charging_number_random) {
            individualResize(individual, charging_number_dist(gen));
        } else {
            individualResize(individual, fixed_charging_number);
        }

        std::uniform_int_distribution<> first_soc(80, 100);
        individual.first_soc = first_soc(gen);

        int last_return_position = 0;
        float elapsed_time = 0;
        int W_total = 0;
        int i = 0;
        while (i < individual.charging_number) {
            std::uniform_int_distribution<> position_dist(0, 1);
            int charging_timing_position = position_dist(gen);
            int return_position = (charging_timing_position == 0) ? 1 : 0;

            int soc_minimum_cycle = calcCycleMax(individual, charging_timing_position, last_return_position, i);
            std::uniform_int_distribution<> cycle_dist(0, soc_minimum_cycle);
            int cycle = cycle_dist(gen);

            individual.time_chromosome[i] = calcTimeChromosome(cycle, last_return_position, charging_timing_position, elapsed_time);
            if (i == 0) {
                individual.soc_charging_start[i] = calcSOCchargingStart(individual.first_soc, cycle, last_return_position, charging_timing_position);
            } else if (last_return_position == 1) {
                individual.soc_charging_start[i] = calcSOCchargingStart(individual.soc_chromosome[i - 1] - E_standby[1] - E_cs[1], cycle, last_return_position, charging_timing_position);
            } else {
                individual.soc_charging_start[i] = calcSOCchargingStart(individual.soc_chromosome[i - 1] - E_cs[0], cycle, last_return_position, charging_timing_position);
            }

            float target_soc_min = individual.soc_charging_start[i] + charging_minimum;
            if (target_soc_min >= 100) { target_soc_min = 100; }
            std::uniform_int_distribution<> target_SOC_dist(target_soc_min, 100);
            individual.soc_chromosome[i] = target_SOC_dist(gen);

            individual.T_span[i][0] = individual.time_chromosome[i] - elapsed_time;
            individual.T_span[i][1] = T_cs[charging_timing_position];
            individual.T_span[i][2] = calcChargingTime(individual.soc_charging_start[i], individual.soc_chromosome[i]);
            individual.T_span[i][3] = (return_position == 0) ? T_cs[0] : T_cs[1] + T_standby[1];

            W_total += calcTotalWork(cycle, last_return_position, charging_timing_position);
            individual.W[i] = W_total;
            individual.E_return[i] = (return_position == 0) ? E_cs[0] : E_cs[1] + E_standby[1];
            individual.charging_position[i] = charging_timing_position;
            individual.return_position[i] = return_position;
            individual.cycle_count[i] = cycle;

            elapsed_time += individual.T_span[i][0] + individual.T_span[i][1] + individual.T_span[i][2] + individual.T_span[i][3];
            last_return_position = return_position;
            ++i;
        }

        fixAndPenalty(individual);
        return individual;
    }

    void TwoTransProblem::generateFirstParents() {
        bool charging_number_random = true;
        for (int i = 0; i < parents.size(); ++i) {
            if (i < 2*parents.size() / 5) {
                charging_number_random = false;
                parents[i] = generateIndividual(charging_number_random, 4);
            } else if (i < 3*parents.size() / 5) {
                charging_number_random = false;
                parents[i] = generateIndividual(charging_number_random, 3);
            } else if (i < 4*parents.size() / 5) {
                charging_number_random = false;
                parents[i] = generateIndividual(charging_number_random, 2);
            } else {
                charging_number_random = true;
                parents[i] = generateIndividual(charging_number_random, 4);
            }
        }
    }

    void TwoTransProblem::generateChildren(bool random) {
        size_t i = 0;
        std::set<std::pair<float, float>> existing_objectives; // 評価値を記録するセット

        // 現在の親集団の評価値をセットに追加
        for (const auto& parent : parents) {
            existing_objectives.insert({parent.f1, parent.f2});
        }

        std::pair<nsgaii::Individual, nsgaii::Individual> selected_parents = randomSelection();
        std::pair<nsgaii::Individual, nsgaii::Individual> child(selected_parents.first.charging_number, selected_parents.second.charging_number);

        while (i < children.size()) {

            int count = 0;
            do {
                // 子個体を生成
                if (random) {
                    selected_parents = randomSelection();
                } else {
                    selected_parents = rankingSelection();
                }
                child = second_crossover(selected_parents);
                calucObjectiveFunction(child.first);
                calucObjectiveFunction(child.second);
                ++count;
                // std::cout << count << std::endl;
                if (count > 1000) break;
            } while (
                existing_objectives.count({child.first.f1, child.first.f2}) > 0 ||
                existing_objectives.count({child.second.f1, child.second.f2}) > 0
            );

            // 子個体の評価値をセットに追加
            existing_objectives.insert({child.first.f1, child.first.f2});
            existing_objectives.insert({child.second.f1, child.second.f2});

            // 子個体を追加
            children[i] = child.first;
            children[i + 1] = child.second;
            i += 2;
        }
    }

    void TwoTransProblem::evaluatePopulation(std::vector<nsgaii::Individual>& population) {
        for (nsgaii::Individual& individual : population) {
            calucObjectiveFunction(individual);
        }
    }

    std::pair<nsgaii::Individual, nsgaii::Individual> TwoTransProblem::crossover(std::pair<nsgaii::Individual, nsgaii::Individual> selected_parents) {
        std::pair<nsgaii::Individual, nsgaii::Individual> child = std::make_pair(nsgaii::Individual(selected_parents.first.charging_number), nsgaii::Individual(selected_parents.second.charging_number));

        child.first.first_soc = selected_parents.first.first_soc;
        child.second.first_soc = selected_parents.second.first_soc;

        std::random_device rd; // ノイズを使用したシード
        std::mt19937 gen(rd()); // メルセンヌ・ツイスタエンジン

        int i = 0;
        int c1_last_return_position = 0;
        int c2_last_return_position = 0;
        float c1_elapsed_time = 0;
        float c2_elapsed_time = 0;
        int c1_W_total = 0;
        int c2_W_total = 0;

        while (i < child.first.charging_number) {
            // std::uniform_int_distribution<> timing_dist(0, 1);
            // int charging_timing_position = timing_dist(gen);
            int c1_charging_timing_position = selected_parents.first.charging_position[i];
            int c2_charging_timing_position = selected_parents.second.charging_position[i];
            int c1_return_position = (c1_charging_timing_position == 0) ? 1 : 0;
            int c2_return_position = (c2_charging_timing_position == 0) ? 1 : 0;

            int small_gen_cycle = std::min(selected_parents.first.cycle_count[i], selected_parents.second.cycle_count[i]);
            int big_gen_cycle = std::max(selected_parents.first.cycle_count[i], selected_parents.second.cycle_count[i]);
            std::pair<int, int> cycle_max = std::make_pair(calcCycleMax(child.first, c1_charging_timing_position, c1_last_return_position, i), calcCycleMax(child.second, c2_charging_timing_position, c2_last_return_position, i)); 
            std::pair<int, int> cycle_min = std::make_pair(small_gen_cycle - std::abs(cycle_max.first - big_gen_cycle), small_gen_cycle - std::abs(cycle_max.second - big_gen_cycle)); 
            if (cycle_min.first < 0) cycle_min.first = 0;
            if (cycle_min.second < 0) cycle_min.second = 0;
            std::pair<int, int> cycle = int_sbx(selected_parents.first.cycle_count[i], selected_parents.second.cycle_count[i], cycle_min, cycle_max);

            child.first.time_chromosome[i] = calcTimeChromosome(cycle.first, c1_last_return_position, c1_charging_timing_position, calcElapsedTime(child.first, i));
            child.second.time_chromosome[i] = calcTimeChromosome(cycle.second, c2_last_return_position, c2_charging_timing_position, calcElapsedTime(child.second, i));
            
            child.first.soc_charging_start[i] = (i == 0) ? calcSOCchargingStart(child.first.first_soc, cycle.first, c1_last_return_position, c1_charging_timing_position) : calcSOCchargingStart(child.first.soc_chromosome[i - 1] - E_cs[c1_last_return_position], cycle.first, c1_last_return_position, c1_charging_timing_position);
            child.second.soc_charging_start[i] = (i == 0) ? calcSOCchargingStart(child.first.first_soc, cycle.second, c2_last_return_position, c2_charging_timing_position) : calcSOCchargingStart(child.second.soc_chromosome[i - 1] - E_cs[c2_last_return_position], cycle.second, c2_last_return_position, c2_charging_timing_position);

            int c1_target_soc_min = std::floor(child.first.soc_charging_start[i] + charging_minimum);
            int c2_target_soc_min = std::floor(child.second.soc_charging_start[i] + charging_minimum);
            
            if (c1_target_soc_min >= 100) { c1_target_soc_min = 100; }
            if (c2_target_soc_min >= 100) { c2_target_soc_min = 100; }

            // std::uniform_int_distribution<> c1_target_SOC_dist(c1_target_soc_min, 100);
            // std::uniform_int_distribution<> c2_target_SOC_dist(c2_target_soc_min, 100);
            std::pair<int, int> soc_target_min = std::make_pair(c1_target_soc_min, c2_target_soc_min);
            std::pair<int, int> soc_target_max = std::make_pair(100, 100);
            std::pair<int, int> soc_target = int_sbx(selected_parents.first.soc_chromosome[i], selected_parents.second.soc_chromosome[i], soc_target_min, soc_target_max);
            child.first.soc_chromosome[i] = soc_target.first;
            child.second.soc_chromosome[i] = soc_target.second;

            child.first.T_span[i][0] = child.first.time_chromosome[i] - c1_elapsed_time;
            child.first.T_span[i][1] = T_cs[c1_charging_timing_position];
            child.first.T_span[i][2] = calcChargingTime(child.first.soc_charging_start[i], child.first.soc_chromosome[i]);
            child.first.T_span[i][3] = (c1_return_position == 0) ? T_cs[c1_return_position] : T_cs[c1_return_position] + T_standby[1];

            c1_W_total += calcTotalWork(cycle.first, c1_last_return_position, c1_charging_timing_position);
            child.first.W[i] = c1_W_total;
            child.first.E_return[i] = (c1_return_position == 0) ? E_cs[c1_return_position] : E_cs[c1_return_position] + E_standby[1];
            child.first.charging_position[i] = c1_charging_timing_position;
            child.first.return_position[i] = c1_return_position;
            child.second.cycle_count[i] = cycle.first;

            child.second.T_span[i][0] = child.second.time_chromosome[i] - c2_elapsed_time;
            child.second.T_span[i][1] = T_cs[c2_charging_timing_position];
            child.second.T_span[i][2] = calcChargingTime(child.second.soc_charging_start[i], child.second.soc_chromosome[i]);
            child.second.T_span[i][3] = (c2_return_position == 0) ? T_cs[c2_return_position] : T_cs[c2_return_position] + T_standby[1];

            c2_W_total += calcTotalWork(cycle.second, c2_last_return_position, c2_charging_timing_position);
            child.second.W[i] = c2_W_total;
            child.second.E_return[i] = (c2_return_position == 0) ? E_cs[c2_return_position] : E_cs[c2_return_position] + E_standby[1];
            child.second.charging_position[i] = c2_charging_timing_position;
            child.second.return_position[i] = c2_return_position;
            child.second.cycle_count[i] = cycle.second;

            c1_elapsed_time += child.first.T_span[i][0] + child.first.T_span[i][1] + child.first.T_span[i][2] + child.first.T_span[i][3];
            c2_elapsed_time += child.second.T_span[i][0] + child.second.T_span[i][1] + child.second.T_span[i][2] + child.second.T_span[i][3];
            c1_last_return_position = c1_return_position;
            c2_last_return_position = c2_return_position;
            ++i;
        }
        while (i < child.second.charging_number) {
            std::random_device rd; // ノイズを使用したシード
            std::mt19937 gen(rd()); // メルセンヌ・ツイスタエンジン

            std::uniform_int_distribution<> timing_dist(0, 1);
            int charging_timing_position = timing_dist(gen);
            int return_position = (charging_timing_position == 0) ? 1 : 0;
            
            int soc_minimum_cycle = calcCycleMax(child.second, charging_timing_position, c2_last_return_position, i);
            std::uniform_int_distribution<> cycle_dist(0, soc_minimum_cycle);
            int cycle = cycle_dist(gen);

            child.second.time_chromosome[i] = calcTimeChromosome(cycle, c2_last_return_position, charging_timing_position, c2_elapsed_time);
            child.second.soc_charging_start[i] = child.second.soc_chromosome[i - 1] - (E_cs[c2_last_return_position] + ((child.second.time_chromosome[i] - c2_elapsed_time) / T_cycle) * E_cycle + E_cs[charging_timing_position]);

            float c2_target_soc_min = child.second.soc_charging_start[i] + charging_minimum;
            if (c2_target_soc_min >= 100) { c2_target_soc_min = 100; }
            std::uniform_int_distribution<> c2_target_SOC_dist(c2_target_soc_min, 100);
            child.second.soc_chromosome[i] = c2_target_SOC_dist(gen);

            child.second.T_span[i][0] = child.second.time_chromosome[i] - c2_elapsed_time;
            child.second.T_span[i][1] = T_cs[charging_timing_position];
            child.second.T_span[i][2] = calcChargingTime(child.second.soc_charging_start[i], child.second.soc_chromosome[i]);
            child.second.T_span[i][3] = (return_position == 0) ? T_cs[return_position] : T_cs[return_position] + T_standby[1];

            c2_W_total += calcTotalWork(cycle, c2_last_return_position, charging_timing_position);
            child.second.W[i] = c2_W_total;
            child.second.E_return[i] = (return_position == 0) ? E_cs[return_position] : E_cs[return_position] + E_standby[1];
            child.second.charging_position[i] = charging_timing_position;
            child.second.return_position[i] = return_position;
            child.second.cycle_count[i] = cycle;
            
            c2_elapsed_time += child.second.T_span[i][0] + child.second.T_span[i][1] + child.second.T_span[i][2] + child.second.T_span[i][3];
            c2_last_return_position = return_position;
            ++i;
        }

        fixAndPenalty(child.first);
        fixAndPenalty(child.second);
        return child;
    }

    std::pair<nsgaii::Individual, nsgaii::Individual> TwoTransProblem::second_crossover(std::pair<nsgaii::Individual, nsgaii::Individual> selected_parents) {
        std::pair<nsgaii::Individual, nsgaii::Individual> child = std::make_pair(nsgaii::Individual(selected_parents.first.charging_number), nsgaii::Individual(selected_parents.second.charging_number));

        child.first.first_soc = selected_parents.first.first_soc;
        child.second.first_soc = selected_parents.second.first_soc;

        std::random_device rd; // ノイズを使用したシード
        std::mt19937 gen(rd()); // メルセンヌ・ツイスタエンジン

        int i = 0;
        int c1_last_return_position = 0;
        int c2_last_return_position = 0;
        float c1_elapsed_time = 0;
        float c2_elapsed_time = 0;
        int c1_W_total = 0;
        int c2_W_total = 0;

        while (i < child.first.charging_number) {
            float c1_min_time = (c1_last_return_position == 0) ? T_standby[0] + c1_elapsed_time : c1_elapsed_time;
            float c2_min_time = (c2_last_return_position == 0) ? T_standby[0] + c2_elapsed_time : c2_elapsed_time;
            int c1_cycle_max = 0;
            int c2_cycle_max = 0;
            int c1_cycle_max_position = 0;
            int c2_cycle_max_position = 0;
            if (calcCycleMax(child.first, 0, c1_last_return_position, i) <= calcCycleMax(child.first, 1, c1_last_return_position, i)) {
                c1_cycle_max = calcCycleMax(child.first, 0, c1_last_return_position, i);
                c1_cycle_max_position = 0;
            } else {
                c1_cycle_max = calcCycleMax(child.first, 1, c1_last_return_position, i);
                c1_cycle_max_position = 1;
            }
            if (calcCycleMax(child.second, 0, c2_last_return_position, i) <= calcCycleMax(child.second, 1, c2_last_return_position, i)) {
                c2_cycle_max = calcCycleMax(child.second, 0, c2_last_return_position, i);
                c2_cycle_max_position = 0;
            } else {
                c2_cycle_max = calcCycleMax(child.second, 1, c2_last_return_position, i);
                c2_cycle_max_position = 1;
            }

            float c1_max_time = c1_cycle_max * T_cycle + c1_elapsed_time;
            float c2_max_time = c2_cycle_max * T_cycle + c2_elapsed_time;
            std::pair<float, float> min_time = std::make_pair(c1_min_time, c2_min_time);
            std::pair<float, float> max_time = std::make_pair(c1_max_time, c2_max_time);

            std::pair<float, float> target_time = float_sbx(selected_parents.first.time_chromosome[i], selected_parents.second.time_chromosome[i], min_time, max_time);

            std::uniform_real_distribution<> time_mutate_dis(0.0, 1.0);
            if (time_mutate_dis(gen) < mutation_probability) {
                target_time.first = timePolynomialMutation(target_time.first, max_time.first, min_time.first);
            }
            if (time_mutate_dis(gen) < mutation_probability) {
                target_time.second = timePolynomialMutation(target_time.second, max_time.second, min_time.second);
            }

            std::pair<int, int>c1_cycle_posit = timeToCycleAndPosition(target_time.first, c1_last_return_position, c1_elapsed_time);
            std::pair<int, int>c2_cycle_posit = timeToCycleAndPosition(target_time.second, c2_last_return_position, c2_elapsed_time);
            if (c1_cycle_posit.first > c1_cycle_max) {
                c1_cycle_posit.first = c1_cycle_max;
                c1_cycle_posit.second = c1_cycle_max_position;
            }
            if (c2_cycle_posit.first > c2_cycle_max) {
                c2_cycle_posit.first = c2_cycle_max;
                c2_cycle_posit.second = c2_cycle_max_position;
            }
            std::pair<int, int> cycle = std::make_pair(c1_cycle_posit.first, c2_cycle_posit.first);
            int c1_charging_timing_position = c1_cycle_posit.second;
            int c1_return_position = (c1_charging_timing_position == 0) ? 1 : 0;
            int c2_charging_timing_position = c2_cycle_posit.second;
            int c2_return_position = (c2_charging_timing_position == 0) ? 1 : 0;
            
            child.first.time_chromosome[i] = calcTimeChromosome(cycle.first, c1_last_return_position, c1_charging_timing_position, c1_elapsed_time);
            child.second.time_chromosome[i] = calcTimeChromosome(cycle.second, c2_last_return_position, c2_charging_timing_position, c2_elapsed_time);

            child.first.soc_charging_start[i] = (i == 0) ? calcSOCchargingStart(child.first.first_soc, cycle.first, c1_last_return_position, c1_charging_timing_position) : calcSOCchargingStart(child.first.soc_chromosome[i - 1] - E_cs[c1_last_return_position], cycle.first, c1_last_return_position, c1_charging_timing_position);
            child.second.soc_charging_start[i] = (i == 0) ? calcSOCchargingStart(child.second.first_soc, cycle.second, c2_last_return_position, c2_charging_timing_position) : calcSOCchargingStart(child.second.soc_chromosome[i - 1] - E_cs[c2_last_return_position], cycle.second, c2_last_return_position, c2_charging_timing_position);
            if (i != 0 && c1_last_return_position == 1) {
                child.first.soc_charging_start[i] = calcSOCchargingStart(child.first.soc_chromosome[i - 1] - E_standby[1] - E_cs[1], cycle.first, c1_last_return_position, c1_charging_timing_position);
            } 
            if (i != 0 && c2_last_return_position == 1) {
                child.second.soc_charging_start[i] = calcSOCchargingStart(child.second.soc_chromosome[i - 1] - E_standby[1] - E_cs[1], cycle.second, c2_last_return_position, c2_charging_timing_position);
            }

            int c1_target_soc_min = std::floor(child.first.soc_charging_start[i] + charging_minimum);
            int c2_target_soc_min = std::floor(child.second.soc_charging_start[i] + charging_minimum);
            if (c1_target_soc_min >= 100) { c1_target_soc_min = 100; }
            if (c2_target_soc_min >= 100) { c2_target_soc_min = 100; }

            std::pair<int, int> soc_target_min = std::make_pair(c1_target_soc_min, c2_target_soc_min);
            std::pair<int, int> soc_target_max = std::make_pair(100, 100);
            std::pair<int, int> soc_target = int_sbx(selected_parents.first.soc_chromosome[i], selected_parents.second.soc_chromosome[i], soc_target_min, soc_target_max);

            std::uniform_real_distribution<> soc_mutate_dis(0.0, 1.0);
            if (soc_mutate_dis(gen) < mutation_probability) {
                soc_target.first = socPolynomialMutation(soc_target.first, soc_target_max.first, soc_target_min.first);
            }
            if (soc_mutate_dis(gen) < mutation_probability) {
                soc_target.second = socPolynomialMutation(soc_target.second, soc_target_max.second, soc_target_min.second);
            }

            child.first.soc_chromosome[i] = soc_target.first;
            child.second.soc_chromosome[i] = soc_target.second;

            child.first.T_span[i][0] = child.first.time_chromosome[i] - c1_elapsed_time;
            child.first.T_span[i][1] = T_cs[c1_charging_timing_position];
            child.first.T_span[i][2] = calcChargingTime(child.first.soc_charging_start[i], child.first.soc_chromosome[i]);
            child.first.T_span[i][3] = (c1_return_position == 0) ? T_cs[c1_return_position] : T_cs[c1_return_position] + T_standby[1];

            c1_W_total += calcTotalWork(cycle.first, c1_last_return_position, c1_charging_timing_position);
            child.first.W[i] = c1_W_total;
            child.first.E_return[i] = (c1_return_position == 0) ? E_cs[c1_return_position] : E_cs[c1_return_position] + E_standby[1];
            child.first.charging_position[i] = c1_charging_timing_position;
            child.first.return_position[i] = c1_return_position;
            child.first.cycle_count[i] = cycle.first;

            child.second.T_span[i][0] = child.second.time_chromosome[i] - c2_elapsed_time;
            child.second.T_span[i][1] = T_cs[c2_charging_timing_position];
            child.second.T_span[i][2] = calcChargingTime(child.second.soc_charging_start[i], child.second.soc_chromosome[i]);
            child.second.T_span[i][3] = (c2_return_position == 0) ? T_cs[c2_return_position] : T_cs[c2_return_position] + T_standby[1];

            c2_W_total += calcTotalWork(cycle.second, c2_last_return_position, c2_charging_timing_position);
            child.second.W[i] = c2_W_total;
            child.second.E_return[i] = (c2_return_position == 0) ? E_cs[c2_return_position] : E_cs[c2_return_position] + E_standby[1];
            child.second.charging_position[i] = c2_charging_timing_position;
            child.second.return_position[i] = c2_return_position;
            child.second.cycle_count[i] = cycle.second;

            c1_elapsed_time += child.first.T_span[i][0] + child.first.T_span[i][1] + child.first.T_span[i][2] + child.first.T_span[i][3];
            c2_elapsed_time += child.second.T_span[i][0] + child.second.T_span[i][1] + child.second.T_span[i][2] + child.second.T_span[i][3];
            c1_last_return_position = c1_return_position;
            c2_last_return_position = c2_return_position;
            ++i;
        }
        while (i < child.second.charging_number) {
            float c2_min_time = (c2_last_return_position == 0) ? T_standby[0] + c2_elapsed_time : c2_elapsed_time;
            int c2_cycle_max = 0;
            int c2_cycle_max_position = 0;
            if (calcCycleMax(child.second, 0, c2_last_return_position, i) <= calcCycleMax(child.second, 1, c2_last_return_position, i)) {
                c2_cycle_max = calcCycleMax(child.second, 0, c2_last_return_position, i);
                c2_cycle_max_position = 0;
            } else {
                c2_cycle_max = calcCycleMax(child.second, 1, c2_last_return_position, i);
                c2_cycle_max_position = 1;
            }

            float c2_max_time = c2_cycle_max * T_cycle + c2_elapsed_time;
            float target_time = selected_parents.second.time_chromosome[i];

            if (target_time < c2_min_time){
                target_time = c2_min_time;
            }

            std::uniform_real_distribution<> time_mutate_dis(0.0, 1.0);
            if (time_mutate_dis(gen) < mutation_probability) {
                target_time = timePolynomialMutation(target_time, c2_max_time, c2_min_time);
            }

            std::pair<int, int>c2_cycle_posit = timeToCycleAndPosition(target_time, c2_last_return_position, c2_elapsed_time);

            int cycle = c2_cycle_posit.first;
            int charging_timing_position = c2_cycle_posit.second;

            if (cycle > c2_cycle_max) {
                cycle = c2_cycle_max;
                charging_timing_position = c2_cycle_max_position;
            }

            int return_position = (charging_timing_position == 0)? 1 : 0;

            child.second.time_chromosome[i] = calcTimeChromosome(cycle, c2_last_return_position, charging_timing_position, c2_elapsed_time);
            if ( c2_last_return_position == 1 ) {
                child.second.soc_charging_start[i] = calcSOCchargingStart(child.second.soc_chromosome[i - 1] - E_standby[1] - E_cs[1], cycle, c2_last_return_position, charging_timing_position);
            } else {
                child.second.soc_charging_start[i] = calcSOCchargingStart(child.second.soc_chromosome[i - 1] - E_cs[0], cycle, c2_last_return_position, charging_timing_position);
            }

            float c2_target_soc_min = std::floor(child.second.soc_charging_start[i] + charging_minimum);
            if (c2_target_soc_min >= 100) { c2_target_soc_min = 100; }

            int target_soc = (selected_parents.second.soc_chromosome[i] > c2_target_soc_min) ? selected_parents.second.soc_chromosome[i] : c2_target_soc_min;

            std::uniform_real_distribution<> soc_mutate_dis(0.0, 1.0);
            if (soc_mutate_dis(gen) < mutation_probability) {
                target_soc = socPolynomialMutation(target_soc, 100, c2_target_soc_min);
            }

            child.second.soc_chromosome[i] = target_soc;

            child.second.T_span[i][0] = child.second.time_chromosome[i] - c2_elapsed_time;
            child.second.T_span[i][1] = T_cs[charging_timing_position];
            child.second.T_span[i][2] = calcChargingTime(child.second.soc_charging_start[i], child.second.soc_chromosome[i]);
            child.second.T_span[i][3] = (return_position == 0) ? T_cs[return_position] : T_cs[return_position] + T_standby[1];

            c2_W_total += calcTotalWork(cycle, c2_last_return_position, charging_timing_position);
            child.second.W[i] = c2_W_total;
            child.second.E_return[i] = (return_position == 0) ? E_cs[return_position] : E_cs[return_position] + E_standby[1];
            child.second.charging_position[i] = charging_timing_position;
            child.second.return_position[i] = return_position;
            child.second.cycle_count[i] = cycle;
            
            c2_elapsed_time += child.second.T_span[i][0] + child.second.T_span[i][1] + child.second.T_span[i][2] + child.second.T_span[i][3];
            c2_last_return_position = return_position;
            ++i;
        }

        fixAndPenalty(child.first);
        fixAndPenalty(child.second);
        return child;
    }

    std::pair<int, int> TwoTransProblem::timeToCycleAndPosition(float& target_time, int& last_return_position, float& elapsed_time) {
        float time = target_time - elapsed_time;
        int cycle = std::floor(time / T_cycle) + 1;
        float cycle_dec = time / T_cycle - std::floor(time / T_cycle);
        int position = 0;

        if (last_return_position == 0) {
            float base_1 = (T_standby[0] + T_move[0] + T_standby[1]) / (2*T_cycle); // 0.41
            float base_2 = T_move[1] / (2*T_cycle) + 2*base_1; // 0.916
            if (cycle_dec <= base_1 || base_2 < cycle_dec) {
                position = 0; // 0.333
            } else {
                position = 1; // 0.833
            }
        } else {
            float base_1 = (T_move[1] + T_standby[0]) / (2*T_cycle); // 0.25
            float base_2 = (T_move[0] + T_standby[1]) / (2*T_cycle) + 2*base_1; // 0.75
            if (cycle_dec <= base_1) {
                position = 1; // 0
                --cycle;
            } else if (base_2 < cycle_dec) {
                position = 1; // 0
            }
            else {
                position = 0; // 0.5
            }
        }
        
        if (cycle < 0) { 
            std::cout << "cycleが0より小さいです" << std::endl;
            std::cout << time << std::endl;
            std::cout << target_time << std::endl;
            std::cout << elapsed_time << std::endl;
        }

        return std::make_pair(cycle, position);
    }

    std::pair<int, int> TwoTransProblem::int_sbx(int& p1, int& p2, std::pair<int, int>& gene_min, std::pair<int, int>& gene_max) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dist(0.0, 1.0);

        float u = dist(gen);
        float beta = (u <= 0.5) 
                    ? pow(2 * u, 1.0 / (eta_sbx + 1)) 
                    : pow(1.0 / (2 * (1 - u)), 1.0 / (eta_sbx + 1));

        int c1 = std::round(0.5 * ((1 + beta) * p1 + (1 - beta) * p2));
        int c2 = std::round(0.5 * ((1 - beta) * p1 + (1 + beta) * p2));

        // 範囲を制限
        c1 = std::max(gene_min.first, std::min(c1, gene_max.first));
        c2 = std::max(gene_min.second, std::min(c2, gene_max.second));

        return std::make_pair(c1, c2);
    }

    std::pair<float, float> TwoTransProblem::float_sbx(float& p1, float& p2, std::pair<float, float>& gene_min, std::pair<float, float>& gene_max) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dist(0.0, 1.0);

        float u = dist(gen);
        float beta = (u <= 0.5) 
                    ? pow(2 * u, 1.0 / (eta_sbx + 1)) 
                    : pow(1.0 / (2 * (1 - u)), 1.0 / (eta_sbx + 1));

        float c1 = 0.5 * ((1 + beta) * p1 + (1 - beta) * p2);
        float c2 = 0.5 * ((1 - beta) * p1 + (1 + beta) * p2);

        // 範囲を制限
        c1 = std::max(gene_min.first, std::min(c1, gene_max.first));
        c2 = std::max(gene_min.second, std::min(c2, gene_max.second));

        return std::make_pair(c1, c2);
    }

    float TwoTransProblem::timePolynomialMutation(float gene, float max_gene, float min_gene) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);

        float u = dis(gen);  
        float delta = 0.0f;

        if (u < 0.5) {
            delta = pow(2.0f * u, 1.0f / (eta_m + 1.0f)) - 1.0f;
        } else {
            delta = 1.0f - pow(2.0f * (1.0f - u), 1.0f / (eta_m + 1.0f));
        }

        float mutated_gene = gene + delta * (max_gene - min_gene);

        return std::clamp(mutated_gene, min_gene, max_gene);
    }

    int TwoTransProblem::socPolynomialMutation(int gene, int max_gene, int min_gene) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);

        float u = dis(gen);  
        float delta;

        if (u < 0.5) {
            delta = pow(2.0f * u, 1.0f / (eta_m + 1.0f)) - 1.0f;
        } else {
            delta = 1.0f - pow(2.0f * (1.0f - u), 1.0f / (eta_m + 1.0f));
        }

        float mutated_gene_float = gene + delta * (max_gene - min_gene);

        int mutated_gene = static_cast<int>(std::round(mutated_gene_float));
        return std::clamp(mutated_gene, min_gene, max_gene);
    }

    float TwoTransProblem::calcTimeChromosome(int& cycle, int& last_return, int& charging_position, float elapsed_time) {
        float time = 0;
        if (cycle == 0) {
            if (last_return == charging_position) {
                // [ last, charge ] [ 0, 0 : 1, 1 ]
                time = (charging_position == 0) ? T_standby[0] : 0; 
            } else {
                // [ last, charge ] [ 1, 0 : 0, 1 ]
                time = (charging_position == 0) ? T_move[1] + T_standby[0] : T_standby[0] + T_move[0] + T_standby[1];
            }
        } else {
            if (last_return == charging_position) {
                // [ last, charge ] [ 0, 0 : 1, 1 ]
                time = (charging_position == 0) ? cycle * T_cycle - T_timing[0] : cycle * T_cycle - T_timing[1];
            } else {
                // [ last, charge ] [ 1, 0 : 0, 1 ]
                time = (charging_position == 0) ? cycle * T_cycle - T_timing[2] : cycle * T_cycle - T_timing[3];
            }
        }
        return time + elapsed_time;
    }

    float TwoTransProblem::calcSOCchargingStart(float first_soc, int& cycle, int& last_return, int& charging_position) {
        float soc_charging_start = 0.0f;
        if (cycle == 0) {
            if (last_return == charging_position) {
                // [ last, charge ] [ 0, 0 : 1, 1 ]
                soc_charging_start = (charging_position == 0) ? first_soc - (E_standby[0] + E_cs[0]) : first_soc - E_cs[1];
            } else {
                // [ last, charge ] [ 1, 0 : 0, 1 ]
                soc_charging_start = (charging_position == 0) ? first_soc - (E_move[1] + E_standby[0] + E_cs[0]) : first_soc - (E_standby[0] + E_move[0] + E_standby[1] + E_cs[1]);
            }
        } else {
            if (last_return == charging_position) {
                // [ last, charge ] [ 0, 0 : 1, 1 ]
                soc_charging_start = (charging_position == 0) ? first_soc - (cycle * E_cycle - E_timing[0] + E_cs[0]) : first_soc - (cycle * E_cycle - E_timing[1] + E_cs[1]);
            } else {
                // [ last, charge ] [ 1, 0 : 0, 1 ]
                soc_charging_start = (charging_position == 0) ? first_soc - (cycle * E_cycle - E_timing[2] + E_cs[0]) : first_soc - (cycle * E_cycle - E_timing[3] + E_cs[1]);
            }
        }
        return soc_charging_start;
    }

    int TwoTransProblem::calcTotalWork(int& cycle, int& last_return, int& charging_position) {
        int W_total = 0;
        if (cycle == 0) {
            if (last_return == charging_position) {
                // [ last, charge ] [ 0, 0 : 1, 1 ]
                W_total = 0; 
            } else {
                // [ last, charge ] [ 1, 0 : 0, 1 ]
                W_total = (charging_position == 0) ? 0 : 1;
            }
        } else {
            if (last_return == charging_position) {
                // [ last, charge ] [ 0, 0 : 1, 1 ]
                W_total = (charging_position == 0) ? cycle - 1 : cycle;
            } else {
                // [ last, charge ] [ 1, 0 : 0, 1 ]
                W_total = (charging_position == 0) ? cycle - 1 : cycle;
            }
        }
        if (charging_position == 0) {
            ++W_total;
        }
        return W_total;
    }

    int TwoTransProblem::calcCycleMax(nsgaii::Individual& individual, int charging_position, int& last_return_position, int& i) {
        int soc_minimum_cycle  = 0;
        if (last_return_position == 1) {
            soc_minimum_cycle = (i == 0) 
                ? std::floor((individual.first_soc - soc_minimum - E_cs[charging_position]) / E_cycle) 
                : std::floor((individual.soc_chromosome[i - 1] - soc_minimum - (E_cs[1] + E_standby[1] + E_cs[charging_position])) / E_cycle);
        } else {
            soc_minimum_cycle  = (i == 0) 
                ? std::floor((individual.first_soc - soc_minimum - E_cs[charging_position]) / E_cycle) 
                : std::floor((individual.soc_chromosome[i - 1] - soc_minimum - (E_cs[0] + E_cs[charging_position])) / E_cycle);
        }
        return soc_minimum_cycle;
    }

    float TwoTransProblem::calcElapsedTime(nsgaii::Individual& individual, int& i) {
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

    void TwoTransProblem::calucObjectiveFunction(nsgaii::Individual& individual)
    {
        calcSOCHiLow(individual);
        individual.f1 = makespan(individual.T_span);
        individual.f2 = soc_HiLowTime(individual.T_SOC_HiLow);
    }

    float TwoTransProblem::makespan(std::vector<std::array<float, 4>>& T_span)
    {
        float makespan = 0;
        for (auto& span : T_span) {
            for (auto& time : span) {
                makespan += time;
            }
        }
        if (makespan < 0) { std::cout << "make: エラー" << std::endl;}
        return makespan;
    }

    float TwoTransProblem::soc_HiLowTime(std::vector<float> T_SOC_HiLow)
    {
        float hi_low_time = 0;
        for (auto& time : T_SOC_HiLow) {
            hi_low_time += time;
        }
        if (hi_low_time < 0) { std::cout << "soc: エラー" << std::endl;}
        return hi_low_time;
    }

    void TwoTransProblem::calcSOCHiLow(nsgaii::Individual& individual) {
        std::array<float, 3> T_socHi = {};
        std::array<float, 3> T_socLow = {};
        float last_final_soc = individual.first_soc;
        for (int i = 0; i < individual.charging_number; ++i) {
            float first_soc = last_final_soc;
            float final_soc = (individual.return_position[i] == 0) ? individual.soc_chromosome[i] - individual.E_return[0] : individual.soc_chromosome[i] - individual.E_return[1] - E_standby[1];

            if (SOC_Hi <= individual.soc_charging_start[i]) {
                T_socHi[0] = individual.T_span[i][0] + individual.T_span[i][1];
            } else if (individual.soc_charging_start[i] <= SOC_Hi && SOC_Hi <= first_soc) {
                T_socHi[0] = ((first_soc - SOC_Hi) / (first_soc - individual.soc_charging_start[i])) * (individual.T_span[i][0] + individual.T_span[i][1]);
            } else {
                T_socHi[0] = 0;
            }
            if (T_socHi[0] < 0) { 
                std::cout << "first_soc: " << first_soc << std::endl;
                std::cout << "individual.soc_charging_start[i]: " << individual.soc_charging_start[i] << std::endl;
                std::cout << "T_socHi[0]: エラー" << std::endl;
            }

            if (SOC_Hi <= individual.soc_charging_start[i]) {
                T_socHi[1] = individual.T_span[i][2];
            } else if (individual.soc_charging_start[i] <= SOC_Hi && SOC_Hi <= individual.soc_chromosome[i]) {
                T_socHi[1] = (individual.soc_chromosome[i] - SOC_Hi) / r_cv;
            } else {
                T_socHi[1] = 0;
            }
            if (T_socHi[1] < 0) { std::cout << "T_socHi[1]: エラー" << std::endl;}

            if (SOC_Hi <= final_soc) {
                T_socHi[2] = individual.T_span[i][3];
            } else if (final_soc <= SOC_Hi && SOC_Hi <= individual.soc_chromosome[i]) {
                T_socHi[2] = ((individual.soc_chromosome[i] - SOC_Hi) / (individual.soc_chromosome[i] - final_soc)) * individual.T_span[i][3];
            } else {
                T_socHi[2] = 0;
            }
            if (T_socHi[2] < 0) { std::cout << "T_socHi[2]: エラー" << std::endl;}

            if (first_soc <= SOC_Low) {
                T_socLow[0] = individual.T_span[i][0] + individual.T_span[i][1];
            } else if (individual.soc_charging_start[i] <= SOC_Low && SOC_Low <= first_soc) {
                T_socLow[0] = ((SOC_Low - individual.soc_charging_start[i]) / (first_soc - individual.soc_charging_start[i])) * (individual.T_span[i][0] + individual.T_span[i][1]);
            } else {
                T_socLow[0] = 0;
            }
            if (T_socLow[0] < 0) { std::cout << "T_socLow[0]: エラー" << std::endl;}

            if (individual.soc_chromosome[i] <= SOC_Low) {
                T_socLow[1] = individual.T_span[i][2];
            } else if (individual.soc_charging_start[i] <= SOC_Low && SOC_Low <= individual.soc_chromosome[i]) {
                T_socLow[1] = (SOC_Low - individual.soc_charging_start[i]) / r_cc;
            } else {
                T_socLow[1] = 0;
            }
            if (T_socLow[1] < 0) { std::cout << "T_socLow[0]: エラー" << std::endl;}

            if (individual.soc_chromosome[i] <= SOC_Low) {
                T_socLow[2] = individual.T_span[i][3];
            } else if (final_soc <= SOC_Low && SOC_Low <= individual.soc_chromosome[i]) {
                T_socLow[2] = ((SOC_Low - final_soc) / (individual.soc_chromosome[i] - final_soc)) * individual.T_span[i][3];
            } else {
                T_socLow[2] = 0;
            }
            if (T_socLow[2] < 0) { std::cout << "T_socLow[0]: エラー" << std::endl;}

            for (int j = 0; j < 3; ++j) {
                individual.T_SOC_HiLow[i] += T_socHi[j] + T_socLow[j];
            }

            last_final_soc = final_soc;
        }

        float first_soc = last_final_soc;
        float final_soc = first_soc - ((individual.T_span[individual.charging_number][0] / T_cycle) * E_cycle);

        if (SOC_Hi <= final_soc) {
            T_socHi[0] = individual.T_span[individual.charging_number][0];
        } else if (final_soc <= SOC_Hi && SOC_Hi <= first_soc) {
            T_socHi[0] = ((first_soc - SOC_Hi) / (first_soc - final_soc)) * individual.T_span[individual.charging_number][0];
        } else {
            T_socHi[0] = 0;
        }
        if (first_soc <= SOC_Low) {
            T_socLow[0] = individual.T_span[individual.charging_number][0];
        } else if (final_soc <= SOC_Low && SOC_Low <= first_soc) {
            T_socLow[0] = ((SOC_Low - final_soc) / (first_soc - final_soc)) * individual.T_span[individual.charging_number][0];
        } else {
            T_socLow[0] = 0;
        }
        if (T_socHi[0] < 0) { 
            std::cout << "first_soc: " << first_soc << std::endl;
            std::cout << "individual.soc_charging_start[i]: " << final_soc << std::endl;
            std::cout << "T_socHi[0]: エラー" << std::endl;
        }
        if (T_socLow[0] < 0) { std::cout << "T_socLow[0]: エラー" << std::endl;}

        individual.T_SOC_HiLow[individual.charging_number] += T_socHi[0] + T_socLow[0];
    }

    float TwoTransProblem::calcChargingTime(float& soc_charging_start, int& soc_target)
    {
        float charging_time = 0;

        if (SOC_cccv <= soc_charging_start){
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
            individual.cycle_count[individual.charging_number] = individual.W[individual.charging_number] - individual.W[individual.charging_number - 1];
            float final_discharge = 0;
            if (individual.return_position[individual.charging_number - 1] == 0) {
                individual.T_span[individual.charging_number][0] = (individual.W[individual.charging_number] - individual.W[individual.charging_number - 1]) * T_cycle - T_move[1];
                individual.T_span[individual.charging_number][1] = 0;
                individual.T_span[individual.charging_number][2] = 0;
                individual.T_span[individual.charging_number][3] = 0;
                final_discharge = (individual.W[individual.charging_number] - individual.W[individual.charging_number - 1]) * E_cycle - E_move[1];
            } else {
                individual.T_span[individual.charging_number][0] = (individual.W[individual.charging_number] - individual.W[individual.charging_number - 1]) * T_cycle;
                individual.T_span[individual.charging_number][1] = 0;
                individual.T_span[individual.charging_number][2] = 0;
                individual.T_span[individual.charging_number][3] = 0;
                final_discharge = (individual.W[individual.charging_number] - individual.W[individual.charging_number - 1]) * E_cycle;
            }
            int span_size = individual.T_span.size();
            if (calcElapsedTime(individual, span_size) > T_max) {
                ++individual.penalty;
            } else if (individual.soc_chromosome[individual.charging_number - 1] - final_discharge < soc_minimum) {
                additionalGen(individual);
                fixAndPenalty(individual);
            }
        } else {
            int new_charging_number = 0;
            for (; new_charging_number < individual.charging_number; ++new_charging_number) {
                if (individual.W[new_charging_number] >= W_target) break;
            }

            individualResize(individual, new_charging_number);
            individual.T_span[new_charging_number][0] = 0;
            individual.W[new_charging_number] = 0;
            individual.cycle_count[new_charging_number] = 0;
            fixAndPenalty(individual);
        }
    }

    void TwoTransProblem::additionalGen(nsgaii::Individual& individual) {
        std::random_device rd; // ノイズを使用したシード
        std::mt19937 gen(rd()); // メルセンヌ・ツイスタエンジン
        int last_return_position = individual.return_position[individual.charging_number - 1];
        float elapsed_time = calcElapsedTime(individual, individual.charging_number);
        int W_total = individual.W[individual.charging_number - 1];
        int i = individual.charging_number;
        individualResize(individual, max_charge_number);
        while (i < individual.charging_number)
        {
            std::uniform_int_distribution<> position_dist(0, 1);
            int charging_timing_position = position_dist(gen);
            int return_position = (charging_timing_position == 0) ? 1 : 0;

            int soc_minimum_cycle = calcCycleMax(individual, charging_timing_position, last_return_position, i);
            std::uniform_int_distribution<> cycle_dist(0, soc_minimum_cycle);
            int cycle = cycle_dist(gen);

            individual.time_chromosome[i] = calcTimeChromosome(cycle, last_return_position, charging_timing_position, elapsed_time);
            if (i == 0) {
                individual.soc_charging_start[i] = calcSOCchargingStart(individual.first_soc, cycle, last_return_position, charging_timing_position);
            } else if (last_return_position == 1) {
                individual.soc_charging_start[i] = calcSOCchargingStart(individual.soc_chromosome[i - 1] - E_standby[1] - E_cs[1], cycle, last_return_position, charging_timing_position);
            } else {
                individual.soc_charging_start[i] = calcSOCchargingStart(individual.soc_chromosome[i - 1] - E_cs[0], cycle, last_return_position, charging_timing_position);
            }

            float target_soc_min = individual.soc_charging_start[i] + charging_minimum;
            if (target_soc_min >= 100) { target_soc_min = 100; }
            std::uniform_int_distribution<> target_SOC_dist(target_soc_min, 100);
            individual.soc_chromosome[i] = target_SOC_dist(gen);

            individual.T_span[i][0] = individual.time_chromosome[i] - elapsed_time;
            individual.T_span[i][1] = T_cs[charging_timing_position];
            individual.T_span[i][2] = calcChargingTime(individual.soc_charging_start[i], individual.soc_chromosome[i]);
            individual.T_span[i][3] = (return_position == 0) ? T_cs[0] : T_cs[1] + T_standby[1];

            W_total += calcTotalWork(cycle, last_return_position, charging_timing_position);
            individual.W[i] = W_total;
            individual.E_return[i] = (return_position == 0) ? E_cs[0] : E_cs[1] + E_standby[1];
            individual.charging_position[i] = charging_timing_position;
            individual.return_position[i] = return_position;
            individual.cycle_count[i] = cycle;

            elapsed_time += individual.T_span[i][0] + individual.T_span[i][1] + individual.T_span[i][2] + individual.T_span[i][3];
            last_return_position = return_position;
            ++i;
        }
    }

    void TwoTransProblem::individualResize(nsgaii::Individual& individual, int new_charging_number) {
        individual.charging_number = new_charging_number;
        individual.time_chromosome.resize(new_charging_number);
        individual.soc_chromosome.resize(new_charging_number);
        individual.T_span.resize(new_charging_number + 1);
        individual.T_SOC_HiLow.resize(new_charging_number + 1);
        individual.E_return.resize(new_charging_number);
        individual.soc_charging_start.resize(new_charging_number);
        individual.W.resize(new_charging_number + 1);
        individual.charging_position.resize(new_charging_number);
        individual.return_position.resize(new_charging_number);
        individual.cycle_count.resize(new_charging_number + 1);
    }

    float TwoTransProblem::calculateHypervolume(const std::vector<nsgaii::Individual>& pareto_front, const float& f1_reference, const float& f2_reference) {
        if (pareto_front.empty()) {
            return 0.0f; // パレートフロントが空の場合、ハイパーボリュームは 0
        }

        // ソート対象をコピー（const 安全性を保つため）
        std::vector<nsgaii::Individual> sorted_pareto_front = pareto_front;

        // f1（第一目的関数）で降順ソート
        std::sort(sorted_pareto_front.begin(), sorted_pareto_front.end(), [](const nsgaii::Individual& a, const nsgaii::Individual& b) {
            return a.f1 > b.f1; // 第一目的関数でソート
        });

        float hypervolume = 0.0f;
        float previous_f1 = f1_reference;

        // 各点で長方形の面積を計算
        for (const auto& individual : sorted_pareto_front) {
            float width = previous_f1 - individual.f1; // f1 軸方向の差
            float height = f2_reference - individual.f2; // f2 軸方向の差

            if (width > 0 && height > 0) { // 有効な領域のみ計算
                hypervolume += width * height;
            }

            previous_f1 = individual.f1; // 次の区間に進む
        }

        return hypervolume;
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