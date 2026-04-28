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
#include "annealing.hpp" // Upewnij się, że masz tu funkcję do initialTemp

using namespace std;

// Kompilacja: g++ -std=c++17 -O3 -fopenmp gridSearch.cpp -o grid_search

int main() {
    namespace fs = std::filesystem;

    // --- PARAMETRY DO GRID SEARCH ---
    // Testujemy różne warianty chłodzenia, liczby epok i mnożnika kroków
    vector<double> coolingRates = {0.90, 0.95, 0.99, 0.995};
    vector<int> epochsList = {1000, 5000, 10000};
    vector<int> stepsMultipliers = {1, 10}; // Kroków na epokę będzie: mnożnik * n
    
    const int NUM_RUNS_PER_CONFIG = 10; // Ile razy powtórzyć każdy wariant, aby uśrednić wynik

    // Wybieramy MAŁY plik do szybkiego testowania parametrów (np. dj38.tsp lub wi29.tsp)
    string targetFile = "data/uy734.tsp"; 

    vector<pair<double, double>> coordinates;
    
    // Szukamy konkretnego pliku
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

    // Dynamicznie liczymy temperaturę początkową
    double initialTemp = getInitialTemperature(dist); // Zależnie od tego jak nazwałaś tę funkcję
    
    cout << "=========================================================\n";
    cout << " GRID SEARCH DLA: " << targetFile << " (Rozmiar n=" << n << ")\n";
    cout << " Wyliczona temperatura poczatkowa: " << initialTemp << "\n";
    cout << "=========================================================\n\n";

    // Nagłówek tabeli wyników
    cout << left << setw(10) << "Cooling" 
         << setw(10) << "Epochs" 
         << setw(15) << "Steps (x n)" 
         << setw(15) << "Avg Cost" 
         << setw(15) << "Best Cost" 
         << setw(15) << "Time (ms)" << "\n";
    cout << string(80, '-') << "\n";

    // Pętle Grid Search
    for(double cr : coolingRates) {
        for(int ep : epochsList) {
            for(int sm : stepsMultipliers) {
                
                int stepsPerEpoch = sm * n;
                vector<long long> costs(NUM_RUNS_PER_CONFIG);
                long long bestCostInConfig = LLONG_MAX;

                // Mierzymy czas dla danej konfiguracji
                auto start_time = chrono::high_resolution_clock::now();

                // Równoległe wykonanie NUM_RUNS_PER_CONFIG prób dla tej konkretnej konfiguracji
                #pragma omp parallel for schedule(dynamic) num_threads(8)
                for (int i = 0; i < NUM_RUNS_PER_CONFIG; ++i) {
                    vector<int> perm = randomPermutation(n);
                    
                    // Odpalamy wyżarzanie
                    long long cost = annealing(dist, perm, initialTemp, cr, ep, stepsPerEpoch);
                    
                    costs[i] = cost;
                }

                auto end_time = chrono::high_resolution_clock::now();
                auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();

                // Podsumowanie wyników z wątków
                long long sumCosts = 0;
                for(long long c : costs) {
                    sumCosts += c;
                    if(c < bestCostInConfig) {
                        bestCostInConfig = c;
                    }
                }
                long long avgCost = sumCosts / NUM_RUNS_PER_CONFIG;

                // Wypisywanie rzędu w tabeli
                cout << left << setw(10) << cr 
                     << setw(10) << ep 
                     << setw(15) << stepsPerEpoch 
                     << setw(15) << avgCost 
                     << setw(15) << bestCostInConfig 
                     << setw(15) << duration << "\n";
            }
        }
    }

    cout << string(80, '-') << "\n";
    cout << "Grid Search zakonczony.\n";

    return 0;
}