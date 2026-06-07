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
#include <numeric>

#include "utils.hpp"
#include "localSearch.hpp"
#include "genetic.hpp"

using namespace std;

// Kompilacja: g++ -std=c++17 -O3 -fopenmp runExperiment.cpp -o run

int main(){
    namespace fs = std::filesystem;

    //KONFIGURACJA
    CrossoverType crossType = PMX; //OX PMX
    string algorithmName = "Genetic_PMX"; 

    for(const auto& entry : fs::directory_iterator("data/")){
        vector<pair<double, double>> coordinates;
        readFile(entry, coordinates);
        if(coordinates.empty()) continue;
        if(entry.path() != "data/ca4663.tsp") continue; 
        //if(entry.path() != "data/eg7146.tsp" && entry.path() != "data/ei8246.tsp" && entry.path() != "data/tz6117.tsp" && entry.path() != "data/ca4663.tsp") continue; 
        //if(entry.path() != "data/mu1979.tsp") continue; 
        // if(entry.path() == "data/eg7146.tsp") continue; 
        // if(entry.path() == "data/ei8246.tsp") continue; 
        // if(entry.path() == "data/tz6117.tsp") continue; 
        // if(entry.path() == "data/mu1979.tsp") continue; 
        
        int n = coordinates.size();
        vector<int> dist = createDistanceMatrix(coordinates);

        // Kontenery na statystyki eksperymentu
        vector<long long> costs;
        long long bestCost = LLONG_MAX;
        vector<int> bestPerm(n);

        // Ustalenie liczby uruchomień  (podstawa = 100)
        int no_runs = 1;

        // Bezpiecznik czasowy dla potężnych plików (algorytm memetyczny jest kosztowny)
        if(entry.path().filename() == "eg7146.tsp" || 
           entry.path().filename() == "tz6117.tsp" || 
           entry.path().filename() == "ei8246.tsp") {
            no_runs = 5; 
        }

        cout << "==================================================\n";
        cout << "File: " << entry.path().filename() << " (Vertices: " << n << ")\n";
        cout << "Running " << algorithmName << " with " << no_runs << " iterations...\n";

        // Główna pętla eksperymentu - sekwencyjnie, ponieważ runIslandGeneticAlgorithm wewnętrznie rozpędza 8 wątków OpenMP.
        for (int i = 0; i < no_runs; ++i) {
            auto startTime = chrono::high_resolution_clock::now();
            
            // Wywołanie algorytmu genetycznego z genetic.hpp
            Individual result = runIslandGeneticAlgorithm(dist, n, crossType);
            
            auto endTime = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();

            costs.push_back(result.cost);
            
            if (result.cost < bestCost) {
                bestCost = result.cost;
                bestPerm = result.tour;
            }

            cout << "  Iteracja " << i + 1 << "/" << no_runs 
                 << " | Cost: " << result.cost 
                 << " | Time: " << duration << " ms\n";
        }

        // Obliczanie średniej wartości kosztu
        long long avgCost = reduce(costs.begin(), costs.end(), 0LL) / costs.size();

        cout << "\nPodsumowanie dla " << entry.path().filename() << ":\n";
        cout << "  Algorytm:     " << algorithmName << "\n";
        cout << "  Average Cost: " << avgCost << "\n";
        cout << "  Best Cost:    " << bestCost << "\n";

        // Zapis najlepszego rozwiązania do pliku .sol
        fs::create_directories("dataSol"); 
        string solFilename = "dataSol/" + entry.path().stem().string() + algorithmName + ".sol";
        ofstream solFile(solFilename);
        if(solFile.is_open()){
            solFile << bestCost << "\n";
            for(auto c : bestPerm) {
                solFile << coordinates[c].first << " " << coordinates[c].second << "\n";
            }
            solFile.close();
            cout << "  Saved best solution to: " << solFilename << "\n";
        }
        else {
            cerr << "  Error opening file for writing: " << solFilename << "\n";
        }
    }
    return 0;
}