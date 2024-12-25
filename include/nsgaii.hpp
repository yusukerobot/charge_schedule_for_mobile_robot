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
      std::vector<float> time_chromosome;
      std::vector<int> soc_chromosome;
      float f1;
      float f2;
      int charging_number;
      int penalty;
      std::vector<std::array<float, 4>> T_span;
      std::vector<float> T_SOC_HiLow;
      std::vector<float> E_return;
      std::vector<float> soc_charging_start;
      std::vector<int> W;
      std::vector<int> charging_position;
      std::vector<int> return_position;
      std::vector<int> cycle_count;
      int fronts_count;
      int first_soc;
      float elapsed_time;
   };

   class ScheduleNsgaii
   {
   public:
      ScheduleNsgaii(const std::string& config_file_path);
      virtual ~ScheduleNsgaii() = default;

      void generateParents();
      virtual void geneChildren();
      void generateCombinedPopulation();

      std::vector<std::vector<int>> nonDominatedSorting(std::vector<Individual>& population);
      void crowdingSorting(std::vector<std::vector<int>> fronts, std::vector<Individual>& population);
      void sortPopulation(std::vector<Individual>& population);
      std::pair<Individual, Individual> rankingSelection();
      std::pair<Individual, Individual> randomSelection();
      virtual std::pair<Individual, Individual> crossover(std::pair<Individual, Individual> selected_parents) = 0;
      void mutation();
      bool dominating(Individual& A, Individual& B);

      virtual void generateFirstParents() = 0;
      virtual void evaluatePopulation(std::vector<nsgaii::Individual>& population) = 0;

      void setEtaSBX(float eta_sbx);
      void setEtaM(float eta_m);
      
      std::vector<Individual> parents;
      std::vector<Individual> children;
      std::vector<Individual> combind_population;

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
      float r_cc;                   // cc充電速度 [%/min]
      float r_cv;                   // cv充電速度 [%/min]
      int charging_minimum;         // 最低充電量 [%]
      float eta_sbx;                // SBX分布指数
      float eta_m;                  // 突然変異分布指数
      float mutation_probability;   // 突然変異確率
   };
} // namespace nsgaii
