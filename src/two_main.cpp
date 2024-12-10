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
    // nsgaii->sortPopulation(nsgaii->parents);

    int max_generation = 1000;
    int current_generation = 0;
    while (current_generation < max_generation) {
        nsgaii->generateChildren();
        nsgaii->evaluatePopulation(nsgaii->children);
        nsgaii->generateCombinedPopulation();
        nsgaii->sortPopulation(nsgaii->combind_population);
        nsgaii->generateParents();
        nsgaii->sortPopulation(nsgaii->parents);
        csvDebugParents(nsgaii->parents, current_generation, base_csv_file_path);
        
        ++current_generation;
    }

    return 0;
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

    // fronts_count == 0 の個体のみデータを書き込む
    for (const auto& individual : parents) {
        if (individual.fronts_count == 0) {
            csvFile << individual.f1 << "," << individual.f2 << "\n";
        }
    }

    csvFile.close();
    // std::cout << "CSVファイルに書き出しました: " << csv_file_path << std::endl;
}
