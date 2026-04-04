#include <iostream>
#include <vector>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <climits>
#include <omp.h>
#include <functional>

#include "utils.hpp"
#include "localSearch.hpp"

using namespace std;

// g++ -std=c++17 -O3 -fopenmp runExperiment.cpp -o run

using SearchAlgorithm = std::function<pair<long long, int>(const vector<int>&, vector<int>&)>;

int main(){
    namespace fs = std::filesystem;

    SearchAlgorithm searchAlgorithm = localSearchFast; 
    string algorithmName = "LocalSearchFast";

    for(const auto& entry : fs::directory_iterator("data/")){
        vector<pair<double, double>> coordinates;
        readFile(entry, coordinates);
        if(coordinates.empty()) continue;
        //if(entry.path() != "data/ei8246.tsp") continue;
        int n = coordinates.size();

        vector<int> dist = createDistanceMatrix(coordinates);

        // Local Search
        vector<long long> costs;
        vector<int> steps;
        long long bestCost = LLONG_MAX;
        vector<int> bestPerm(n);

        #pragma omp parallel num_threads(8) // similar to 20 threads but can kinda use the computer while running
        {
            vector<long long> localCosts;
            vector<int> localSteps;
            long long localBestCost = LLONG_MAX;
            vector<int> localBestPerm(n);
            int no_runs = ceil(sqrt(n));

            // only for full local search
            //if(entry.path() == "data/eg7146.tsp") no_runs = 8;
            //if(entry.path() == "data/ei8246.tsp") no_runs = 8;
            //if(entry.path() == "data/tz6117.tsp") no_runs = 8;

            //#pragma omp for nowait
            #pragma omp for schedule(dynamic) nowait
            for (int i = 0; i < no_runs; ++i) {
                auto perm = randomPermutation(n);
                auto [costR, stepR] = searchAlgorithm(dist, perm);
                localCosts.push_back(costR);
                localSteps.push_back(stepR);
                if (costR < localBestCost) {
                    localBestCost = costR;
                    localBestPerm = perm;
                }
                #pragma omp critical(logging)
                {
                    cout << i+1 << "/" << no_runs << "  ";
                }
            }

            #pragma omp critical
            {
                costs.insert(costs.end(), localCosts.begin(), localCosts.end());
                steps.insert(steps.end(), localSteps.begin(), localSteps.end());
                if (localBestCost < bestCost) {
                    bestCost = localBestCost;
                    bestPerm = localBestPerm;
                }
            }
        }

        long long avgCost = reduce(costs.begin(), costs.end(), 0LL) / costs.size();
        double avgSteps = static_cast<double>(reduce(steps.begin(), steps.end(), 0)) / steps.size();

        cout << "File: " << entry.path().filename() << "\n";

        cout << "Local Search \n";
        cout << "Average Cost: " << avgCost << "\n";
        cout << "Average Steps: " << avgSteps << "\n";
        //for data file .sol
        string solFilename = "dataSol/" + entry.path().stem().string() + algorithmName + ".sol";
        ofstream solFile(solFilename);
        if(solFile.is_open()){
            solFile << bestCost << "\n";
            for(auto c : bestPerm) solFile << coordinates[c].first << " " << coordinates[c].second << "\n";
            solFile.close();
        }
        else cerr <<"Error opening file for writing: " << solFilename << "\n";
    }
    return 0;
}