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
#include <random>

#include "utils.hpp"
#include "annealing.hpp"

using namespace std;

// g++ -std=c++17 -O3 -fopenmp runExperiment.cpp -o run

using SearchAlgorithm = std::function<long long(const vector<int>&, vector<int>&, double, double, int, int)>;

int main(){
    namespace fs = std::filesystem;

    // SETUP
    SearchAlgorithm searchAlgorithm = annealing; 
    string algorithmName = "Annealing";

    for(const auto& entry : fs::directory_iterator("data/")){
        vector<pair<double, double>> coordinates;
        readFile(entry, coordinates);
        if(coordinates.empty()) continue;
        //if(entry.path() != "data/zi929.tsp") continue;
        int n = coordinates.size();

        vector<int> dist = createDistanceMatrix(coordinates);

        // Annealing
        vector<long long> costs;
        long long bestCost = LLONG_MAX;
        vector<int> bestPerm(n);
        
        //for mst
        std::random_device rd;  // a seed source for the random number engine
        std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
        std::uniform_int_distribution<> distrib(0, n-1);

        double initialTemp = getInitialTemperature(dist);
        cout<< "Initial temperature: " << initialTemp << "\n";

        #pragma omp parallel num_threads(8) // similar to 20 threads but can kinda use the computer while running
        {
            vector<long long> localCosts;
            long long localBestCost = LLONG_MAX;
            vector<int> localBestPerm(n);
            int no_runs = 100; //ceil(sqrt(n));

            // only for full local search
            if(entry.path() == "data/eg7146.tsp") no_runs = 8;
            if(entry.path() == "data/ei8246.tsp") no_runs = 8;
            if(entry.path() == "data/tz6117.tsp") no_runs = 8;

            //#pragma omp for nowait
            #pragma omp for schedule(dynamic) nowait
            for (int i = 0; i < no_runs; ++i) {

                vector<int> perm;
                perm = randomPermutation(n);

                long long costR = searchAlgorithm(dist, perm, initialTemp, 0.995, 1000, 10*n);
                localCosts.push_back(costR);
                if (costR < localBestCost) {
                    localBestCost = costR;
                    localBestPerm = perm;
                }
                #pragma omp critical(logging)
                {
                    std::cout << i+1 << "/" << no_runs << "  ";
                }
            }

            #pragma omp critical
            {
                costs.insert(costs.end(), localCosts.begin(), localCosts.end());
                if (localBestCost < bestCost) {
                    bestCost = localBestCost;
                    bestPerm = localBestPerm;
                }
            }
        }

        long long avgCost = reduce(costs.begin(), costs.end(), 0LL) / costs.size();

        std::cout << "File: " << entry.path().filename() << "\n";

        std::cout << "Local Search \n";
        std::cout << "Average Cost: " << avgCost << "\n";
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