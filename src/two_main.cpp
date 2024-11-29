#include <memory>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <fstream>

#include "two_point_trans_schedule.hpp"

void csvDebugParents(std::vector<nsgaii::Individual> parents, int generations, std::string csv_file_path);

int main()
{
   std::string csv_file_path = "../data/parents.csv";
   std::string config_file_path = "../params/two_charge_schedule.yaml";
   std::unique_ptr<charge_schedule::TwoTransProblem> nsgaii = std::make_unique<charge_schedule::TwoTransProblem>(config_file_path);

   nsgaii->generateFirstCombinedPopulation();
   nsgaii->evaluatePopulation(nsgaii->combind_population);
   nsgaii->generateParents();
   csvDebugParents(nsgaii->parents, 1, csv_file_path);

   return 0;
}

void csvDebugParents(std::vector<nsgaii::Individual> parents, int generations, std::string csv_file_path) {
   std::ofstream csvFile(csv_file_path, std::ios::app);
   if (!csvFile) {
      std::cerr << "ファイルを開けませんでした！" << std::endl;
   }

   csvFile << "第" << generations << "世代\n";
   csvFile << "f1" << "," << "f2\n";
   for (auto& individual : parents) {
      csvFile << individual.f1 << "," << individual.f2 << "\n";
   }

   csvFile.close();
   std::cout << "CSVファイルに書き出しました。" << std::endl;
}
