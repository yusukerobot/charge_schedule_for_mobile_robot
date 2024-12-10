#include <memory>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <ctime>

#include "two_point_trans_schedule.hpp"

void csvDebugParents(std::vector<nsgaii::Individual>& parents, int& generations, const std::string& base_csv_file_path);

int main()
{
   // ベースとなるCSVファイルのパス（日時を付与するためのテンプレート）
   std::string base_csv_file_path = "../data/parents";

   // YAML設定ファイルのパス
   std::string config_file_path = "../params/two_charge_schedule.yaml";

   std::unique_ptr<charge_schedule::TwoTransProblem> nsgaii = std::make_unique<charge_schedule::TwoTransProblem>(config_file_path);

   nsgaii->generateFirstParents();
   nsgaii->evaluatePopulation(nsgaii->parents);
   nsgaii->sortPopulation(nsgaii->parents);

   // std::cout << " parents size: " << nsgaii->parents.size() << std::endl;
   // for (int i = 0; i < nsgaii->parents.size(); ++i) {
   //    std::cout << i << " : " << nsgaii->parents[i].f1 << ", " << nsgaii->parents[i].f2 << std::endl;
   // }

   // for (int i = 0; i < 1000; ++i) {
      nsgaii->generateChildren();
      nsgaii->evaluatePopulation(nsgaii->children);
      // std::cout << " children size: " << nsgaii->children.size() << std::endl;
      // for (int i = 0; i < nsgaii->children.size(); ++i) {
      //    std::cout << i << " : " << nsgaii->children[i].f1 << ", " << nsgaii->children[i].f2 << std::endl;
      // }
      nsgaii->generateCombinedPopulation();
      nsgaii->sortPopulation(nsgaii->combind_population);
      nsgaii->generateParents();
      // csvDebugParents(nsgaii->parents, i, base_csv_file_path);
   // }

   return 0;
}

void csvDebugParents(std::vector<nsgaii::Individual>& parents, int& generations, const std::string& base_csv_file_path) {
    // 現在の日時を取得
    std::time_t now = std::time(nullptr);
    char date_time[20];
    std::strftime(date_time, sizeof(date_time), "%Y-%m-%d_%H-%M-%S", std::localtime(&now));

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

    // 各個体のデータを書き込む
    for (auto& individual : parents) {
        csvFile << individual.f1 << "," << individual.f2 << "\n";
    }

    csvFile.close();
    std::cout << "CSVファイルに書き出しました: " << csv_file_path << std::endl;
}
