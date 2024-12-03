#include <vector>
#include <memory>
#include <random>
#include <iostream>
#include <yaml-cpp/yaml.h>

#include "nsgaii.hpp"

namespace nsgaii
{
   Individual::Individual(const int& chromosome_size)
   : time_chromosome(chromosome_size, 0), soc_chromosome(chromosome_size, 0), f1(0), f2(0), charging_number(0), penalty(0)
   {
      T_span.resize(chromosome_size + 1);
      T_SOC_HiLow.resize(chromosome_size + 1);
      E_return.resize(chromosome_size + 1, 0);
      soc_charging_start.resize(chromosome_size + 1, 0);
      W.resize(chromosome_size + 1, 0);
   }

   ScheduleNsgaii::ScheduleNsgaii(const std::string& config_file_path)
   {
      YAML::Node node;
      try {
         node = YAML::LoadFile(config_file_path);
      } catch (const YAML::Exception& e) {
         std::cerr << "YAMLファイルの読み込みに失敗しました: " << e.what() << std::endl;
         throw std::runtime_error("YAML読み込みエラー");
      }

      YAML::Node config = node["charge_schedule"];

      // visited_number の設定と検証
      visited_number = config["visited_number"].as<int>();
      if (visited_number <= 0) {
         std::cerr << "visited_numberが無効です: " << visited_number << std::endl;
         throw std::invalid_argument("visited_number is invalid");
      }

      // サイズのリサイズと初期化
      T_move.resize(visited_number);
      T_standby.resize(visited_number);
      T_cs.resize(visited_number);
      E_move.resize(visited_number);
      E_standby.resize(visited_number);
      E_cs.resize(visited_number);

      // YAMLからデータを読み込み
      for (size_t i = 0; i < visited_number; ++i) {
         T_move[i] = config["T_move"][i].as<float>();
         T_standby[i] = config["T_standby"][i].as<float>();
         T_cs[i] = config["T_cs"][i].as<float>();
         E_move[i] = config["E_move"][i].as<float>();
         E_standby[i] = config["E_standby"][i].as<float>();
         E_cs[i] = config["E_cs"][i].as<float>();
      }

      // その他のパラメータの設定
      population_size = config["population_size"].as<int>();
      T_max = config["T_max"].as<int>();
      max_charge_number = config["max_charge_number"].as<int>();
      W_target = config["W_target"].as<int>();
      SOC_Hi = config["SOC_Hi"].as<int>();
      SOC_Low = config["SOC_Low"].as<int>();
      SOC_cccv = config["SOC_cccv"].as<int>();
      r_cc = config["r_cc"].as<float>();
      r_cv = config["r_cv"].as<float>();
      q_min = config["q_min"].as<int>();

      // 個体の初期化
      parents.resize(population_size, Individual(max_charge_number));
      children.resize(population_size, Individual(max_charge_number));
      combind_population.resize(2*population_size, Individual(2 * max_charge_number));


   }

   void ScheduleNsgaii::generateParents()
   {
      std::vector<std::vector<int>> fronts = nonDominatedSorting();
      // for (auto& front : fronts) {
      //    for (int& individual : front) {
      //       std::cout << individual << " ";
      //    }
      //    std::cout << std::endl;
      // }
      crowdingSorting(fronts);
      for (int i = 0; i < population_size; i++)
      {
         parents[i] = combind_population[i];
         // std::cout << parents[i].charging_number << std::endl;
      }
   }

   void ScheduleNsgaii::generateChildren()
   {
      std::pair<Individual, Individual> selected_parents = rankingSelection();
      crossover(selected_parents);
      mutation();
   }

   void ScheduleNsgaii::generateCombinedPopulation()
   {
      combind_population.clear();
      combind_population.reserve(parents.size() + children.size());
      combind_population.insert(combind_population.end(), parents.begin(), parents.end());
      combind_population.insert(combind_population.end(), children.begin(), children.end());
   }

   std::vector<std::vector<int>> ScheduleNsgaii::nonDominatedSorting()
   {
      std::vector<std::vector<int>> fronts;
      std::vector<int> Np(combind_population.size(), 0);
      std::vector<std::set<int>> Sp(combind_population.size());

      for (size_t i = 0; i < combind_population.size(); ++i) 
      {
         for (size_t j = 0; j < combind_population.size(); ++j) 
         {
            if (i == j) continue;
            if (dominating(combind_population[i], combind_population[j])) 
            {
               Sp[i].insert(j);
            }
            else if (dominating(combind_population[j], combind_population[i])) 
            {
               ++Np[i];
            }
         }
      }

      std::vector<int> first_front;
      for (size_t i = 0; i < combind_population.size(); ++i) 
      {
         if (Np[i] == 0) 
         {
            first_front.push_back(i);
         }
      }
      fronts.push_back(first_front);

      size_t current_front = 0;
      int fronts_individual_count = first_front.size();
      while (fronts_individual_count < combind_population.size())
      {
         std::vector<int> next_front;
         for (int individual : fronts[current_front]) 
         {
            for (int dominated : Sp[individual]) 
            {
               --Np[dominated];
               if (Np[dominated] == 0) 
               {
                  next_front.push_back(dominated);
               }
            }
         }
         if (!next_front.empty()) 
         {
            fronts.push_back(next_front);
            ++current_front;
            fronts_individual_count += next_front.size();
         }
      }
      return fronts;
   }

   void ScheduleNsgaii::crowdingSorting(std::vector<std::vector<int>>& fronts)
   {
      for (auto& front : fronts) 
      {
         if (front.size() < 2) continue;

         std::vector<float> crowding_distance(front.size(), 0.0f);

         for (size_t obj = 0; obj < 2; ++obj) 
         {
            std::sort(front.begin(), front.end(), [&](int a, int b) {
               return (obj == 0) 
                  ? combind_population[a].f1 <= combind_population[b].f1
                  : combind_population[a].f2 <= combind_population[b].f2;
            });

            crowding_distance[0] = std::numeric_limits<float>::infinity();
            crowding_distance[front.size() - 1] = std::numeric_limits<float>::infinity();

            for (size_t i = 1; i < front.size() - 1; ++i) 
            {
               float diff = (obj == 0)
                  ? combind_population[front[i + 1]].f1 - combind_population[front[i - 1]].f1
                  : combind_population[front[i + 1]].f2 - combind_population[front[i - 1]].f2;

               float range = (obj == 0)
                  ? combind_population[front.back()].f1 - combind_population[front.front()].f1
                  : combind_population[front.back()].f2 - combind_population[front.front()].f2;

               if (range > 0) {
                  crowding_distance[i] += diff / range;
               } else {
                  crowding_distance[i] += 0.0;
               }
            }
         }

         std::vector<std::pair<int, float>> indexed_front;
         for (size_t i = 0; i < front.size(); ++i) {
            indexed_front.emplace_back(front[i], crowding_distance[i]);
         }

         std::sort(indexed_front.begin(), indexed_front.end(), [](const auto& a, const auto& b) {
            return a.second > b.second;
         });

         front.clear();
         for (const auto& elem : indexed_front) {
            front.push_back(elem.first);
         }
      }

      std::vector<Individual> sorted_population;
      for (const auto& front : fronts) 
      {
         for (int individual : front) 
         {
            sorted_population.push_back(combind_population[individual]);
         }
      }
      
      combind_population = std::move(sorted_population);
   }

   std::pair<Individual, Individual> ScheduleNsgaii::rankingSelection()
   {
      std::pair<Individual, Individual> selected_parents = std::make_pair(Individual(max_charge_number), Individual(max_charge_number));
      return selected_parents;      
   }

   void ScheduleNsgaii::crossover(std::pair<Individual, Individual> selected_parents)
   {
      int a =0;
   }

   void ScheduleNsgaii::mutation()
   {
      int a =0;
   }

   bool ScheduleNsgaii::dominating(Individual& A, Individual& B)
   {
      bool all_less_or_equal = (A.f1 <= B.f1 && A.f2 <= B.f2);
      bool any_less = (A.f1 < B.f1 || A.f2 < B.f2);
      return all_less_or_equal && any_less;
   }
} // namespace nsgaii
