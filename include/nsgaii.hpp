#pragma once

#include <vector>
#include <array>
#include <memory>
#include <string>

namespace nsgaii
{
   struct Individual
   {
      Individual(const int& chromosome_size);
      std::vector<int> time_chromosome;
      std::vector<int> soc_chromosome;
      float f1;
      float f2;
      int charging_number;
      std::vector<std::array<float, 4>> T_span;
      std::vector<std::array<float, 2>> T_SOC_HiLow;
      std::vector<int> W;
   };

   class ScheduleNsgaii
   {
   public:
      ScheduleNsgaii(const std::string& config_file_path);
      virtual ~ScheduleNsgaii() = default;

      void generateParents();
      void generateChildren();
      void generateCombinedPopulation();

      std::vector<std::vector<int>> nonDominatedSorting();
      void crowdingSorting(std::vector<std::vector<int>>& fronts);
      std::pair<Individual, Individual> rankingSelection();
      void crossover(std::pair<Individual, Individual> selected_parents);
      void mutation();
      bool dominating(Individual& A, Individual& B);

      virtual void generateFirstCombinedPopulation() = 0;
      virtual void evaluatePopulation(std::vector<nsgaii::Individual>& population) = 0;

   protected:
      std::vector<float> T_move;    // 移動時間 [min]
      std::vector<float> T_standby; // 待機時間 [min]
      std::vector<float> T_cs;      // 充電ステーションまでの移動時間 [min]
      std::vector<float> E_move;    // 移動中の放電量 [%]
      std::vector<float> E_standby; // 待機中の放電量 [%]
      std::vector<float> E_cs;      // 充電ステーションまでの移動中の放電量 [%]
      int visited_number;           // 訪問先の数
      int population_size;          // 個体群サイズ
      int T_max;                    // 最大作業時間
      int max_charge_number;        // 最大充電回数
      int W_target;                 // 目標タスク量 [回]
      int SOC_Hi;                   // SOC高領域閾値 [%]
      int SOC_Low;                  // SOC低領域閾値 [%]
      int SOC_cccv;                 // cc-cv充電切り替え閾値 [%]
      int r_cc;                     // cc充電速度 [%/min]
      int r_cv;                     // cv充電速度 [%/min]
      int q_min;                    // 最低充電量 [%]

      std::vector<Individual> parents;
      std::vector<Individual> children;
      std::vector<Individual> combind_population;
   };
} // namespace nsgaii
