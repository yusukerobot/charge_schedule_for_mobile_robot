#include <memory>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <ctime>
#include <vector>

#include "two_point_trans_schedule.hpp"

void csvDebugParents(std::vector<nsgaii::Individual>& parents, int generation, const std::string& base_csv_file_path);
void runGenerationLoop(std::unique_ptr<charge_schedule::TwoTransProblem>& nsgaii, const std::string& base_csv_file_path, const std::vector<int>& eta_values, int max_generation);

int main()
{
    std::string config_file_path = "../params/two_charge_schedule.yaml";
    std::unique_ptr<charge_schedule::TwoTransProblem> nsgaii = std::make_unique<charge_schedule::TwoTransProblem>(config_file_path);

    // etaの最小値、最大値、ステップ数を指定
    int eta_min = 1;
    int eta_max = 20;
    int eta_step = 1;

    // etaの値を作成
    std::vector<int> eta_values;
    for (int eta = eta_min; eta <= eta_max; eta += eta_step) {
        eta_values.push_back(eta);
    }

    // 生成ループを実行
    runGenerationLoop(nsgaii, "../data/sbx/eta", eta_values, 100);

    return 0;
}

void runGenerationLoop(std::unique_ptr<charge_schedule::TwoTransProblem>& nsgaii, const std::string& base_csv_file_path, const std::vector<int>& eta_values, int max_generation)
{
    // 最初の親を生成
    nsgaii->generateFirstParents();
    nsgaii->evaluatePopulation(nsgaii->parents);
    nsgaii->sortPopulation(nsgaii->parents);
    std::vector<nsgaii::Individual> first_parents = nsgaii->parents;

    for (int eta : eta_values) {
        // 各etaに対して処理を実行
        std::string eta_csv_file_path = base_csv_file_path + std::to_string(eta);
        nsgaii->setEtaSBX(eta);  // SBXで使用するetaを設定
        int current_generation = 0;

        // 親を元にした世代ごとの進行
        nsgaii->parents.clear();
        nsgaii->parents = first_parents;

        while (current_generation < max_generation) {
            csvDebugParents(nsgaii->parents, current_generation, eta_csv_file_path);
            nsgaii->generateChildren(false);  // randomフラグはfalse
            nsgaii->evaluatePopulation(nsgaii->children);
            nsgaii->generateCombinedPopulation();
            nsgaii->sortPopulation(nsgaii->combind_population);
            nsgaii->generateParents();

            ++current_generation;
        }
        csvDebugParents(nsgaii->parents, current_generation, eta_csv_file_path);
    }
}

void csvDebugParents(std::vector<nsgaii::Individual>& parents, int generation, const std::string& base_csv_file_path)
{
    // 現在の日時を取得
    std::time_t now = std::time(nullptr);
    char date_time[20];
    std::strftime(date_time, sizeof(date_time), "%Y-%m-%d_%H-%M", std::localtime(&now));

    // 日時を含むファイルパスを生成
    std::string csv_file_path = base_csv_file_path + ".csv";

    std::ofstream csvFile(csv_file_path, std::ios::app);
    if (!csvFile) {
        std::cerr << "ファイルを開けませんでした！" << std::endl;
        return;
    }

    csvFile << "第" << generation << "世代\n";

    for (const auto& individual : parents) {
        csvFile << "f1" << "," << "f2" << "," << "first_soc" << "," << "front\n";
        csvFile << individual.f1 << "," << individual.f2 << "," << individual.first_soc << "," << individual.fronts_count << "\n";
        csvFile << "time" << "\n";
        for (size_t i = 0; i < individual.time_chromosome.size(); ++i) {
            csvFile << individual.time_chromosome[i];
            if (i != individual.time_chromosome.size() - 1) {
                csvFile << ",";
            }
        }
        csvFile << "\n";

        // 'soc'データを書き込む
        csvFile << "soc" << "\n";
        for (size_t i = 0; i < individual.soc_chromosome.size(); ++i) {
            csvFile << individual.soc_chromosome[i];
            if (i != individual.soc_chromosome.size() - 1) {
                csvFile << ",";
            }
        }
        csvFile << "\n";
    }

    csvFile.close();
}
