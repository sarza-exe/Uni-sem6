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
#include <iomanip>

#include "utils.hpp"
#include "tspSolvers.hpp" 
// Przyjmuje (distMatrix, solution, tabuLength, maxIterWithoutImprovement, neighborsToCheck)

// g++ -std=c++17 -O3 -fopenmp gridSearchTabu.cpp -o tabu

using namespace std;

int main() {
    namespace fs = std::filesystem;

    // Wybieramy MAŁY plik do szybkiego testowania parametrów
    string targetFile = "data/qa194.tsp"; 

    vector<pair<double, double>> coordinates;
    bool fileFound = false;
    for(const auto& entry : fs::directory_iterator("data/")) {
        if(entry.path() == targetFile) {
            readFile(entry, coordinates);
            fileFound = true;
            break;
        }
    }

    if(!fileFound || coordinates.empty()) {
        cerr << "Nie znaleziono pliku " << targetFile << " lub jest pusty!" << endl;
        return 1;
    }

    int n = coordinates.size();
    vector<int> dist = createDistanceMatrix(coordinates);

    // --- PARAMETRY DO GRID SEARCH DLA TABU SEARCH ---
    // tabuModifiers to mnożnik dla 'n'. Np. 0.5 oznacza tabuLength = n / 2
    vector<double> tabuModifiers = {0.25, 0.5, 1.0, 2.0}; 
    vector<int> stagnationLimits = {100, 500, 1000}; // maxIterWithoutImprovement
    vector<int> neighborsMultipliers = {2, 5, 10}; // ile sasiadow sprawdzamy: mult * n
    
    const int NUM_RUNS_PER_CONFIG = 10; // Uśrednienie z 10 przebiegów
    
    cout << "========================================================================\n";
    cout << " GRID SEARCH TABU SEARCH DLA: " << targetFile << " (Rozmiar n=" << n << ")\n";
    cout << "========================================================================\n\n";

    // Nagłówek tabeli wyników
    cout << left << setw(15) << "Tabu Len" 
         << setw(15) << "Max Stagn." 
         << setw(15) << "Neighbors" 
         << setw(15) << "Avg Cost" 
         << setw(15) << "Best Cost" 
         << setw(15) << "Time (ms)" << "\n";
    cout << string(85, '-') << "\n";

    // Pętle Grid Search
    for(double tabuMod : tabuModifiers) {
        for(int stagLim : stagnationLimits) {
            for(int nm : neighborsMultipliers) {
                
                int tabuLength = std::max(1, static_cast<int>(tabuMod * n));
                int neighborsToCheck = nm * n;
                
                vector<long long> costs(NUM_RUNS_PER_CONFIG);
                long long bestCostInConfig = LLONG_MAX;

                auto start_time = chrono::high_resolution_clock::now();

                // Równoległe przebiegi dla uśrednienia
                #pragma omp parallel for schedule(dynamic) num_threads(8)
                for (int i = 0; i < NUM_RUNS_PER_CONFIG; ++i) {
                    // Losowa permutacja startowa (lub MST jeśli wolisz od razu dobre starty!)
                    vector<int> perm = randomPermutation(n);
                    
                    long long cost = tabuSearch(dist, perm, tabuLength, stagLim, neighborsToCheck);
                    
                    costs[i] = cost;
                }

                auto end_time = chrono::high_resolution_clock::now();
                auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();

                long long sumCosts = 0;
                for(long long c : costs) {
                    sumCosts += c;
                    if(c < bestCostInConfig) {
                        bestCostInConfig = c;
                    }
                }
                long long avgCost = sumCosts / NUM_RUNS_PER_CONFIG;

                // Wypisywanie wiersza
                cout << left 
                     << setw(15) << tabuLength 
                     << setw(15) << stagLim 
                     << setw(15) << neighborsToCheck 
                     << setw(15) << avgCost 
                     << setw(15) << bestCostInConfig 
                     << setw(15) << duration << "\n";
            }
        }
    }

    cout << string(85, '-') << "\n";
    cout << "Grid Search Tabu zakonczony.\n";

    return 0;
}