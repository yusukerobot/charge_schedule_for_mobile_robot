#include <vector>
#include <memory>
#include <random>
#include <iostream>
#include <yaml-cpp/yaml.h>

#include "nsgaii.hpp"

namespace nsgaii {
   Individual::Individual(const int& chromosome_size)
   : time_chromosome(chromosome_size, 0), 
   soc_chromosome(chromosome_size, 0), 
   f1(0), 
   f2(0), 
   charging_number(chromosome_size), 
   penalty(0), 
   fronts_count(0), 
   first_soc(100),
   elapsed_time(0.0f)
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
      eta_sbx = config["eta_sbx"].as<float>();
      eta_m = config["eta_m"].as<float>();
      mutation_probability = config["mutation_probability"].as<float>();

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

   void ScheduleNsgaii::geneChildren() {
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
      if (population.empty()) {
         return {};
      }

      size_t n = population.size();
      std::vector<std::vector<int>> fronts;
      std::vector<int> Np(n, 0); // 各個体が支配されている数
      std::vector<std::vector<int>> Sp(n); // 各個体が支配している個体のリスト

      // 支配関係の計算
      for (size_t i = 0; i < n; ++i) {
         for (size_t j = i + 1; j < n; ++j) {
               if (dominating(population[i], population[j])) {
                  Sp[i].push_back(j);
                  if (j >= n) { // 範囲チェック
                     std::cerr << "Error: Invalid index j (" << j << ") for population size " << n << std::endl;
                     throw std::out_of_range("Invalid index in Sp.");
                  }
                  ++Np[j];
               } else if (dominating(population[j], population[i])) {
                  Sp[j].push_back(i);
                  if (i >= n) { // 範囲チェック
                     std::cerr << "Error: Invalid index i (" << i << ") for population size " << n << std::endl;
                     throw std::out_of_range("Invalid index in Sp.");
                  }
                  ++Np[i];
               }
         }
      }

      // 最初のフロントの作成
      std::vector<int> first_front;
      for (size_t i = 0; i < n; ++i) {
         if (Np[i] == 0) {
               first_front.push_back(i);
               population[i].fronts_count = 0;
         }
      }
      fronts.push_back(first_front);

      // 残りのフロントの計算
      size_t processed_individuals = first_front.size();
      while (processed_individuals < n) {
         std::vector<int> next_front;
         for (int individual : fronts.back()) {
               for (int dominated : Sp[individual]) {
                  --Np[dominated];
                  if (Np[dominated] == 0) {
                     next_front.push_back(dominated);
                     population[dominated].fronts_count = fronts.size();
                  }
               }
         }
         if (next_front.empty()) {
               break;
         }
         fronts.push_back(next_front);
         processed_individuals += next_front.size();
      }

      // 範囲外チェック
      for (const auto& front : fronts) {
         for (int index : front) {
               if (index < 0 || static_cast<size_t>(index) >= population.size()) {
                  std::cerr << "Error: Invalid index in fronts: " << index << std::endl;
                  throw std::out_of_range("Invalid index in fronts.");
               }
         }
      }

      return fronts;
   }

   void ScheduleNsgaii::crowdingSorting(std::vector<std::vector<int>> fronts, std::vector<Individual>& population) {
   //  std::cout << "Before sorting in crowdingSorting:\n";
   //  for (size_t i = 0; i < fronts.size(); ++i) {
   //      for (int index : fronts[i]) {
   //          std::cout << index << " ";
   //      }
   //      std::cout << "\n";
   //  }
   //  std::cout << "\n";

    for (auto& front : fronts) {
        if (front.size() < 2) continue;

        // インデックスが範囲内であることを確認
        for (int index : front) {
            if (index < 0 || index >= population.size()) {
                std::cerr << "Error: Index " << index << " is out of range. Population size: " << population.size() << std::endl;
                throw std::out_of_range("Index is out of range in population.");
            }
        }

        // クラウド距離を計算するために、各インデックスに対して値を保持
        std::vector<std::pair<int, float>> indexed_front;
        for (size_t i = 0; i < front.size(); ++i) {
            indexed_front.push_back({front[i], 0.0f});
        }

        // 2つのオブジェクトに対して、クラウド距離を計算
        for (size_t obj = 0; obj < 2; ++obj) {
            // オブジェクトごとにソートするためにラムダ式を使って比較
            std::sort(indexed_front.begin(), indexed_front.end(), [&](const std::pair<int, float>& a, const std::pair<int, float>& b) {
                if (obj == 0) {
                    return population[a.first].f1 < population[b.first].f1;
                } else {
                    return population[a.first].f2 < population[b.first].f2;
                }
            });

            // クラウド距離の計算
            indexed_front.front().second = std::numeric_limits<float>::infinity();
            indexed_front.back().second = std::numeric_limits<float>::infinity();

            for (size_t i = 1; i < indexed_front.size() - 1; ++i) {
                float diff = (obj == 0)
                    ? population[indexed_front[i + 1].first].f1 - population[indexed_front[i - 1].first].f1
                    : population[indexed_front[i + 1].first].f2 - population[indexed_front[i - 1].first].f2;

                float range = (obj == 0)
                    ? population[indexed_front.back().first].f1 - population[indexed_front.front().first].f1
                    : population[indexed_front.back().first].f2 - population[indexed_front.front().first].f2;

                if (range > 0) {
                    indexed_front[i].second += diff / range;
                }
            }
        }

        // クラウド距離でソート
        std::sort(indexed_front.begin(), indexed_front.end(), [](const std::pair<int, float>& a, const std::pair<int, float>& b) {
            return a.second > b.second;
        });

        // frontを更新
        front.clear();
        for (const auto& elem : indexed_front) {
            front.push_back(elem.first);
        }
    }

    // ソートされたpopulationを作成
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
      // std::cout << "渡し" << std::endl;
      // for (auto& front : fronts) {
      //    for (auto& index : front) {
      //       std::cout << index << " ";
      //    }
      //    std::cout << std::endl;
      // }
      // std::cout << std::endl;

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

      // 親が異なる評価値を持つまで繰り返す
      do {
         for (int i = 0; i < 2; ++i) {
               int first = select_dist(gen);
               int second = select_dist(gen);
               Individual& selected = (first <= second) ? parents[first] : parents[second];
               (i == 0 ? selected_parents.first : selected_parents.second) = selected;
         }
      } while (selected_parents.first.f1 == selected_parents.second.f1 &&
               selected_parents.first.f2 == selected_parents.second.f2);

      // charging_numberを比較して必要に応じて入れ替え
      if (selected_parents.first.charging_number > selected_parents.second.charging_number) {
         std::swap(selected_parents.first, selected_parents.second);
      }

      return selected_parents;
   }

   std::pair<Individual, Individual> ScheduleNsgaii::randomSelection() {
      std::pair<Individual, Individual> selected_parents = std::make_pair(Individual(max_charge_number), Individual(max_charge_number));

      std::random_device rd; // ノイズを使用したシード
      std::mt19937 gen(rd()); // メルセンヌ・ツイスタエンジン
      std::uniform_int_distribution<> select_dist(0, parents.size() - 1);
      // 最初の親を選択
      int first_index = select_dist(gen);
      selected_parents.first = parents[first_index];

      int second_index;

      // 評価値が異なる親を選択するまで繰り返す
      do {
         second_index = select_dist(gen);
      } while (
         second_index == first_index || // 同じ親を選ばない
         (parents[first_index].f1 == parents[second_index].f1 && 
            parents[first_index].f2 == parents[second_index].f2) // 評価値が異なるかチェック
      );

      selected_parents.second = parents[second_index];

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

   void ScheduleNsgaii::setEtaSBX(float eta_sbx) {
      this->eta_sbx = eta_sbx;
   }

   void ScheduleNsgaii::setEtaM(float eta_m) {
      this->eta_m = eta_m;
   }
} // namespace nsgaii
