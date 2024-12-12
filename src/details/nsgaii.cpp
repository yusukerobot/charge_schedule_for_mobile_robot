#include <vector>
#include <memory>
#include <random>
#include <iostream>
#include <yaml-cpp/yaml.h>

#include "nsgaii.hpp"

namespace nsgaii {
   Individual::Individual(const int& chromosome_size)
   : time_chromosome(chromosome_size, 0), soc_chromosome(chromosome_size, 0), f1(0), f2(0), charging_number(chromosome_size), penalty(0), fronts_count(0), first_soc(100) 
   {
      T_span.resize(chromosome_size + 1);
      T_SOC_HiLow.resize(chromosome_size + 1);
      E_return.resize(chromosome_size, 0);
      soc_charging_start.resize(chromosome_size, 0);
      W.resize(chromosome_size + 1, 0);
      charging_position.resize(chromosome_size, 0);
      return_position.resize(chromosome_size, 0);
      cycle_count.resize(chromosome_size + 1, 0);
   }

   ScheduleNsgaii::ScheduleNsgaii(const std::string& config_file_path) {
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
      charging_minimum = config["charging_minimum"].as<int>();

      // 個体の初期化
      parents.resize(population_size, Individual(max_charge_number));
      children.resize(population_size, Individual(max_charge_number));
      combind_population.resize(2*population_size, Individual(max_charge_number));


   }

   void ScheduleNsgaii::generateParents() {
      for (int i = 0; i < parents.size(); i++) {
         parents[i] = combind_population[i];
      }
   }

   void ScheduleNsgaii::generateChildren() {
      size_t i = 0;
      while (i < children.size()) {
         std::pair<Individual, Individual> selected_parents = rankingSelection();
         std::pair<Individual, Individual> child = crossover(selected_parents);
         // mutation();
         children[i] = child.first;
         children[i + 1] = child.second;
         i += 2;

         std::cout << "--- selected parents ---" << std::endl;

         std::cout << "first " << std::endl;
         std::cout << "  time: ";
         for (float& time : selected_parents.first.time_chromosome) {
            std::cout << time << " ";
         }
         std::cout << std::endl;
         std::cout << "  soc: ";
         for (auto& soc : selected_parents.first.soc_chromosome) {
            std::cout << soc << " ";
         }
         std::cout << std::endl;
         std::cout << "  W: ";
         for (auto& W : selected_parents.first.W) {
            std::cout << W << " ";
         }
         std::cout << std::endl;
         std::cout << "  T_span: ";
         for (auto& span : selected_parents.first.T_span) {
            float elapsed_time = 0;
            for (auto& time : span) {
               elapsed_time += time;
            }
            std::cout << elapsed_time << " ";
         }
         std::cout << std::endl;
         std::cout << "  f1: " << selected_parents.first.f1 << ", f2: "<< selected_parents.first.f2 << std::endl;

         std::cout << "second "<< std::endl;
         std::cout << "  time: ";
         for (float& time : selected_parents.second.time_chromosome) {
            std::cout << time << " ";
         }
         std::cout << std::endl;
         std::cout << "  soc: ";
         for (auto& soc : selected_parents.second.soc_chromosome) {
            std::cout << soc << " ";
         }
         std::cout << std::endl;
         std::cout << "  W: ";
         for (auto& W : selected_parents.second.W) {
            std::cout << W << " ";
         }
         std::cout << std::endl;
         std::cout << "  T_span: ";
         for (auto& span : selected_parents.second.T_span) {
            float elapsed_time = 0;
            for (auto& time : span) {
               elapsed_time += time;
            }
            std::cout << elapsed_time << " ";
         }
         std::cout << std::endl;
         std::cout << "  f1: " << selected_parents.second.f1 << ", f2: "<< selected_parents.second.f2 << std::endl;
         std::cout << "------" << std::endl;

         std::cout << "--- child ---" << std::endl;
         std::cout << "first: " << std::endl;
         std::cout << "  time: ";
         for (float& time : child.first.time_chromosome) {
            std::cout << time << " ";
         }
         std::cout << std::endl;
         std::cout << "  soc: ";
         for (auto& soc : child.first.soc_chromosome) {
            std::cout << soc << " ";
         }
         std::cout << std::endl;
         std::cout << "  W: ";
         for (auto& W : child.first.W) {
            std::cout << W << " ";
         }
         std::cout << std::endl;
         std::cout << "  T_span: ";
         for (auto& span : child.first.T_span) {
            float elapsed_time = 0;
            for (auto& time : span) {
               elapsed_time += time;
            }
            std::cout << elapsed_time << " ";
         }
         std::cout << std::endl;

         std::cout << "second: " << std::endl;
         std::cout << "  time: ";
         for (float& time : child.second.time_chromosome) {
            std::cout << time << " ";
         }
         std::cout << std::endl;
         std::cout << "  soc: ";
         for (auto& soc : child.second.soc_chromosome) {
            std::cout << soc << " ";
         }
         std::cout << std::endl;
         std::cout << "  W: ";
         for (auto& W : child.second.W) {
            std::cout << W << " ";
         }
         std::cout << std::endl;
         std::cout << "  T_span: ";
         for (auto& span : child.second.T_span) {
            float elapsed_time = 0;
            for (auto& time : span) {
               elapsed_time += time;
            }
            std::cout << elapsed_time << " ";
         }
         std::cout << std::endl;
         std::cout << "------" << std::endl;

         // std::cout << child.first.penalty << std::endl;
         // std::cout << child.second.penalty << std::endl;
      }
   }

   void ScheduleNsgaii::generateCombinedPopulation() {
      combind_population.clear();
      combind_population.reserve(parents.size() + children.size());
      combind_population.insert(combind_population.end(), parents.begin(), parents.end());
      combind_population.insert(combind_population.end(), children.begin(), children.end());
   }

   std::vector<std::vector<int>> ScheduleNsgaii::nonDominatedSorting(std::vector<Individual>& population) {
      std::vector<std::vector<int>> fronts;
      std::vector<int> Np(population.size(), 0); // 各個体が他から支配されている数
      std::vector<std::set<int>> Sp(population.size()); // 各個体が支配している個体のリスト

      // 各個体間の支配関係を判定
      for (size_t i = 0; i < population.size(); ++i) {
         for (size_t j = 0; j < population.size(); ++j) {
               if (i == j) continue;
               if (dominating(population[i], population[j])) {
                  Sp[i].insert(j); // i が j を支配
               } else if (dominating(population[j], population[i])) {
                  ++Np[i]; // j が i を支配
               }
         }
      }

      // 最初のフロントを作成
      std::vector<int> first_front;
      for (size_t i = 0; i < population.size(); ++i) {
         if (Np[i] == 0) {
               first_front.push_back(i);
               population[i].fronts_count = 0; // フロント番号を設定
         }
      }
      fronts.push_back(first_front);

      // 残りのフロントを計算
      size_t current_front = 0;
      int fronts_individual_count = first_front.size();
      while (fronts_individual_count < population.size()) {
         std::vector<int> next_front;
         for (int individual : fronts[current_front]) {
               for (int dominated : Sp[individual]) {
                  --Np[dominated]; // 支配されている数を減らす
                  if (Np[dominated] == 0) {
                     next_front.push_back(dominated);
                      population[dominated].fronts_count = current_front + 1;
                  }
               }
         }
         if (!next_front.empty()) {
               fronts.push_back(next_front);
               ++current_front;
               fronts_individual_count += next_front.size();
         }
      }
      return fronts;
   }

   void ScheduleNsgaii::crowdingSorting(std::vector<std::vector<int>>& fronts, std::vector<Individual>& population) {
      for (auto& front : fronts) {
         if (front.size() < 2) continue;

         std::vector<float> crowding_distance(front.size(), 0.0f);

         // for (int& index : front) {
         //    std::cout << index << ", ";
         // }
         // std::cout << std::endl;

         for (size_t obj = 0; obj < 2; ++obj) {
            std::sort(front.begin(), front.end(), [&](int a, int b) {
               if (a < 0 || a >= population.size()) {
                  std::cerr << "Error: a (" << a << ") is out of range. Population size: " << population.size() << std::endl;
                  throw std::out_of_range("Index 'a' is out of range in population.");
               }
               if (b < 0 || b >= population.size()) {
                  std::cerr << "Error: b (" << b << ") is out of range. Population size: " << population.size() << std::endl;
                  throw std::out_of_range("Index 'b' is out of range in population.");
               }
               return (obj == 0) 
                  ? population[a].f1 <= population[b].f1
                  : population[a].f2 <= population[b].f2;
            });

            crowding_distance[0] = std::numeric_limits<float>::infinity();
            crowding_distance[front.size() - 1] = std::numeric_limits<float>::infinity();

            for (size_t i = 1; i < front.size() - 1; ++i) {
               float diff = (obj == 0)
                  ? population[front[i + 1]].f1 - population[front[i - 1]].f1
                  : population[front[i + 1]].f2 - population[front[i - 1]].f2;

               float range = (obj == 0)
                  ? population[front.back()].f1 - population[front.front()].f1
                  : population[front.back()].f2 - population[front.front()].f2;

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
      for (const auto& front : fronts) {
         for (int individual : front) {
            sorted_population.push_back(population[individual]);
         }
      }
      
      population = std::move(sorted_population);
   }

   void ScheduleNsgaii::sortPopulation(std::vector<Individual>& population) {
      std::vector<std::vector<int>> fronts = nonDominatedSorting(population);
      crowdingSorting(fronts, population);
      std::stable_sort(population.begin(), population.end(),
                  [](const Individual& a, const Individual& b) {
                        return a.penalty < b.penalty; // penaltyが少ないものを優先
                  });
      for (auto& individual : population) {
         individual.fronts_count += individual.penalty;
      }
   }

   std::pair<Individual, Individual> ScheduleNsgaii::rankingSelection() {
      std::pair<Individual, Individual> selected_parents = std::make_pair(Individual(max_charge_number), Individual(max_charge_number));

      std::random_device rd; // ノイズを使用したシード
      std::mt19937 gen(rd()); // メルセンヌ・ツイスタエンジン
      std::uniform_int_distribution<> select_dist(0, parents.size() - 1);

      for (int i = 0; i < 2; ++i) {
         int first = select_dist(gen);
         int second = select_dist(gen);         
         Individual& selected = (first <= second) ? parents[first] : parents[second];
         (i == 0 ? selected_parents.first : selected_parents.second) = selected;
      }

      // charging_numberを比較して必要に応じて入れ替え
      if (selected_parents.first.charging_number > selected_parents.second.charging_number) {
         std::swap(selected_parents.first, selected_parents.second);
      }

      return selected_parents;
   }

   void ScheduleNsgaii::mutation() {
      int a =0;
   }

   bool ScheduleNsgaii::dominating(Individual& A, Individual& B) {
      bool all_less_or_equal = (A.f1 <= B.f1 && A.f2 <= B.f2);
      bool any_less = (A.f1 < B.f1 || A.f2 < B.f2);
      return all_less_or_equal && any_less;
   }
} // namespace nsgaii
