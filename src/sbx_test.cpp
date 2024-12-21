#include <memory>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <ctime>

#include "two_point_trans_schedule.hpp"

void csvDebugParents(std::vector<nsgaii::Individual>& parents, int& generations, const std::string& base_csv_file_path);
void outputscreen(std::pair<nsgaii::Individual, nsgaii::Individual>& parents,std::pair<nsgaii::Individual, nsgaii::Individual>& children);


int main()
{
   // ベースとなるCSVファイルのパス（日時を付与するためのテンプレート）
   std::string base_csv_file_path = "../data/sbx_test";

   // YAML設定ファイルのパス
   std::string config_file_path = "../params/two_charge_schedule.yaml";

   std::unique_ptr<charge_schedule::TwoTransProblem> nsgaii = std::make_unique<charge_schedule::TwoTransProblem>(config_file_path);

   int current_generation = 0;
   nsgaii->generateFirstParents();
   nsgaii->evaluatePopulation(nsgaii->parents);
   nsgaii->sortPopulation(nsgaii->parents);
   csvDebugParents(nsgaii->parents, current_generation, base_csv_file_path);

   nsgaii->setEtaSBX(2.0);
   nsgaii->generateChildren(random);
   csvDebugParents(nsgaii->children, current_generation, base_csv_file_path);

   nsgaii->setEtaSBX(20.0);
   nsgaii->generateChildren(random);
   csvDebugParents(nsgaii->children, current_generation, base_csv_file_path);
}

void outputscreen(std::pair<nsgaii::Individual, nsgaii::Individual>& parents,std::pair<nsgaii::Individual, nsgaii::Individual>& children) {
    std::cout << "--- First ---" << std::endl;
    std::cout << "p1: " << std::endl;
    // std::cout << "  time: ";
    // for (float& time : parents.first.time_chromosome) {
    //     std::cout << time << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  soc: ";
    // for (auto& soc : parents.first.soc_chromosome) {
    //     std::cout << soc << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  W: ";
    // for (auto& W : parents.first.W) {
    //     std::cout << W << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  T_span: ";
    // for (auto& span : parents.first.T_span) {
    //     float elapsed_time = 0;
    //     for (auto& time : span) {
    //         elapsed_time += time;
    //     }
    //     std::cout << elapsed_time << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  E_return: ";
    // for (auto& e : parents.first.E_return) {
    //     std::cout << e << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  soc_charging_start: ";
    // for (auto& soc : parents.first.soc_charging_start) {
    //     std::cout << soc << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  charging_position: ";
    // for (auto& pos : parents.first.charging_position) {
    //     std::cout << pos << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  return_position: ";
    // for (auto& pos : parents.first.return_position) {
    //     std::cout << pos << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  cycle_count: ";
    // for (auto& count : parents.first.cycle_count) {
    //     std::cout << count << " ";
    // }
    // std::cout << std::endl;
    std::cout << "f1: " << parents.first.f1 << " f2: " << parents.first.f2 << std::endl;

    std::cout << "c1: " << std::endl;
    // std::cout << "  time: ";
    // for (float& time : children.first.time_chromosome) {
    //     std::cout << time << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  soc: ";
    // for (auto& soc : children.first.soc_chromosome) {
    //     std::cout << soc << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  W: ";
    // for (auto& W : children.first.W) {
    //     std::cout << W << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  T_span: ";
    // for (auto& span : children.first.T_span) {
    //     float elapsed_time = 0;
    //     for (auto& time : span) {
    //         elapsed_time += time;
    //     }
    //     std::cout << elapsed_time << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  E_return: ";
    // for (auto& e : children.first.E_return) {
    //     std::cout << e << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  soc_charging_start: ";
    // for (auto& soc : children.first.soc_charging_start) {
    //     std::cout << soc << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  charging_position: ";
    // for (auto& pos : children.first.charging_position) {
    //     std::cout << pos << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  return_position: ";
    // for (auto& pos : children.first.return_position) {
    //     std::cout << pos << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  cycle_count: ";
    // for (auto& count : children.first.cycle_count) {
    //     std::cout << count << " ";
    // }
    // std::cout << std::endl;
    std::cout << "f1: " << children.first.f1 << " f2: " << children.first.f2 << std::endl;
    std::cout << std::endl;

    std::cout << "--- Second ---" << std::endl;
    std::cout << "p2: " << std::endl;
    // std::cout << "  time: ";
    // for (float& time : parents.second.time_chromosome) {
    //     std::cout << time << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  soc: ";
    // for (auto& soc : parents.second.soc_chromosome) {
    //     std::cout << soc << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  W: ";
    // for (auto& W : parents.second.W) {
    //     std::cout << W << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  T_span: ";
    // for (auto& span : parents.second.T_span) {
    //     float elapsed_time = 0;
    //     for (auto& time : span) {
    //         elapsed_time += time;
    //     }
    //     std::cout << elapsed_time << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  E_return: ";
    // for (auto& e : parents.second.E_return) {
    //     std::cout << e << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  soc_charging_start: ";
    // for (auto& soc : parents.second.soc_charging_start) {
    //     std::cout << soc << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  charging_position: ";
    // for (auto& pos : parents.second.charging_position) {
    //     std::cout << pos << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  return_position: ";
    // for (auto& pos : parents.second.return_position) {
    //     std::cout << pos << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  cycle_count: ";
    // for (auto& count : parents.second.cycle_count) {
    //     std::cout << count << " ";
    // }
    // std::cout << std::endl;
    std::cout << "f1: " << parents.second.f1 << " f2: " << parents.second.f2 << std::endl;

    std::cout << "c2: " << std::endl;
    // std::cout << "  time: ";
    // for (float& time : children.second.time_chromosome) {
    //     std::cout << time << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  soc: ";
    // for (auto& soc : children.second.soc_chromosome) {
    //     std::cout << soc << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  W: ";
    // for (auto& W : children.second.W) {
    //     std::cout << W << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  T_span: ";
    // for (auto& span : children.second.T_span) {
    //     float elapsed_time = 0;
    //     for (auto& time : span) {
    //         elapsed_time += time;
    //     }
    //     std::cout << elapsed_time << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  E_return: ";
    // for (auto& e : children.second.E_return) {
    //     std::cout << e << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  soc_charging_start: ";
    // for (auto& soc : children.second.soc_charging_start) {
    //     std::cout << soc << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  charging_position: ";
    // for (auto& pos : children.second.charging_position) {
    //     std::cout << pos << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  return_position: ";
    // for (auto& pos : children.second.return_position) {
    //     std::cout << pos << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "  cycle_count: ";
    // for (auto& count : children.second.cycle_count) {
    //     std::cout << count << " ";
    // }
    // std::cout << std::endl;
    std::cout << "f1: " << children.second.f1 << " f2: " << children.second.f2 << std::endl;
    std::cout << "--- --- ---" << std::endl;

}

void csvDebugParents(std::vector<nsgaii::Individual>& parents, int& generations, const std::string& base_csv_file_path) {
   // 現在の日時を取得
   std::time_t now = std::time(nullptr);
   char date_time[20];
   std::strftime(date_time, sizeof(date_time), "%Y-%m-%d_%H-%M", std::localtime(&now));

   // 日時を含むファイルパスを生成
   std::string csv_file_path = base_csv_file_path + "_" + date_time + ".csv";

   std::ofstream csvFile(csv_file_path, std::ios::app);
   if (!csvFile) {
      std::cerr << "ファイルを開けませんでした！" << std::endl;
      return;
   }

    // ヘッダーを書き込む
   csvFile << "第" << generations << "世代\n";
   csvFile << "f1" << "," << "f2\n";
   for (const auto& individual : parents) {
      csvFile << individual.f1 << "," << individual.f2 << "," << individual.fronts_count << "\n";
   }

   csvFile.close();
}