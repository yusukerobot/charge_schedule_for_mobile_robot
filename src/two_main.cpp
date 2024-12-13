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
    int current_generation = 0;

    // int max_generation = 100;
    // while (current_generation < max_generation) {
        csvDebugParents(nsgaii->parents, current_generation, base_csv_file_path);
        nsgaii->generateChildren();
        nsgaii->evaluatePopulation(nsgaii->children);
        csvDebugParents(nsgaii->children, current_generation, base_csv_file_path);

        // std::cout << "--- child ---" << std::endl;
        //  for (auto& individual : nsgaii->children) {
        //     std::cout << "  time: ";
        //     for (auto& time : individual.time_chromosome) {
        //         std::cout << time << " ";
        //     }
        //     std::cout << std::endl;
        //     std::cout << "  soc: ";
        //     for (auto& soc : individual.soc_chromosome) {
        //         std::cout << soc << " ";
        //     }
        //     std::cout << std::endl;
        //     std::cout << "  f1: " << individual.f1 << ", f2: "<< individual.f2 << std::endl;
        //  }

        // nsgaii->generateCombinedPopulation();
        // nsgaii->sortPopulation(nsgaii->combind_population);
        // nsgaii->generateParents();
        // nsgaii->sortPopulation(nsgaii->parents);
        // ++current_generation;
    // }
    // csvDebugParents(nsgaii->parents, current_generation, base_csv_file_path);

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
    // for (const auto& individual : parents) {
    //     if (individual.fronts_count == 0) {
    //         csvFile << individual.f1 << "," << individual.f2 << "\n";
    //     }
    // }

    // for (int i = 0; i < parents.size(); ++i) {
    //     csvFile << parents[i].f1 << "," << parents[i].f2 << "," <<parents[i].fronts_count << "\n";
    // }

    // if (generations == 0 || generations == 99){
    //     csvFile << "第" << generations << "世代\n";
    //     csvFile << "f1" << "," << "f2\n";
        for (const auto& individual : parents) {
            csvFile << individual.f1 << "," << individual.f2 << "," << individual.fronts_count << "\n";
        }
    // }

    csvFile.close();
    // std::cout << "CSVファイルに書き出しました: " << csv_file_path << std::endl;
}
