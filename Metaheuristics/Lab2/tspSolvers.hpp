#pragma once 

#include <iostream>
#include <vector>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <random>
#include <chrono>
#include <algorithm>
#include <climits>
#include <omp.h>
#include <atomic>
#include <functional>

#include "utils.hpp"

using namespace std;

/**
 * @brief runs simulated annealing on the given solution and distance matrix
 * @param distMatrix The flat (row-major) distance matrix of the TSP instance
 * @param solution The initial tour (will be modified in place)
 * @param initialTemp The initial temperature for the annealing process
 * @param coolingRate The rate at which the temperature decreases
 * @param epochs The number of epochs to run
 * @param stepsPerEpoch The number of steps to take per epoch
 * @return The cost of the final solution
 */
long long annealing(const vector<int>& distMatrix, vector<int>& solution, double initialTemp = 100.0, double coolingRate = 0.95, int epochs = 1000, int stepsPerEpoch = 100) {
    // 1. Wylosuj rozwiązanie początkowe X.
    // 2. Wybierz losowe rozwiązanie X′ znajdujące się w sąsiedztwie X.
    // 3. Jeśli X′ jest lepsze to je przyjmij (X := X′). W przeciwnym razie, wyznacz prawdopo-
    // dobieństwo przyjęcia nowego rozwiązania używając wzoru e(f (X)−f (X′))/T i z tym praw-
    // dopodobieństwem X := X′.
    // 4. Jeśli nie wykonano jeszcze odpowiedniej liczby prób w obrębie danej epoki, wróć do
    // punktu 2.
    // 5. Zmniejsz temperaturę.
    // 6. Jeśli nie osiągnięto jeszcze warunku stopu, wróć do punktu 2
    long long currCost = calculateCost(distMatrix, solution);
    const int n = solution.size();

    mt19937 mt{};
    std::uniform_int_distribution randN{0, n-1};
    std::uniform_real_distribution<double> randProb(0.0, 1.0);  

    double temp = initialTemp;

    for(int epoch = 0; epoch < epochs; epoch++){
        for(int trial = 0; trial < stepsPerEpoch; trial++){
            int i = randN(mt);
            int j = randN(mt);
            if(i > j) swap(i,j);
            if(i == j || (i == 0 && j == n - 1)) continue;

            int idx_prev_i = ((i - 1 + n) % n);
            int node_prev_i = solution[idx_prev_i];
            int node_i = solution[i];
            int next_j = (j + 1) % n;
            int node_j = solution[j];
            int node_next_j = solution[next_j];

            long long delta = -distMatrix[node_prev_i * n + node_i] 
                                - distMatrix[node_j * n + node_next_j]
                                + distMatrix[node_prev_i * n + node_j] 
                                + distMatrix[node_i * n + node_next_j];            

            if (delta < 0 || exp(-delta / temp) > randProb(mt)) {
                currCost += delta;
                std::reverse(solution.begin() + i, solution.begin() + j + 1);
            }
        }

        temp *= coolingRate;
    }
    return calculateCost(distMatrix, solution);
}


/**
 * runs tabu search on the given solution and distance matrix
 * @param distMatrix The flat (row-major) distance matrix of the TSP instance
 * @param solution The initial tour (will be modified in place)
 * @param tabuLength The length of the tabu (number of iterations a move remains tabu)
 * @param maxIterWithoutImprovement The maximum number of iterations without improvement
 * @param neighborhoodSize The number of random neighbors to consider in each iteration (multiplier for n)
 * @return The cost of the final solution

 */
long long tabuSearch(const vector<int>& distMatrix, vector<int>& solution, const int tabuLength = 500, const int maxIterWithoutImprovement = 500, const int neighborhoodSize = 10) {
    long long currCost = calculateCost(distMatrix, solution);
    long long globalBestCost = currCost;
    vector<int> bestSolution = solution;

    const int n = solution.size();
    long long iter = 0;
    int iterWithoutImprovement = 0;

    random_device rd;
    mt19937 mt{rd()};
    uniform_int_distribution randN{0, n-1};

    vector<int> tabuList(n * n, 0); 

    while(iterWithoutImprovement < maxIterWithoutImprovement) {
        iter++;
        long long bestDelta = LLONG_MAX; // Inicjalizujemy nieskończonościa
        int newI = -1; 
        int newJ = -1;

        for (int k = 0; k < neighborhoodSize * n; k++) {
            int i = randN(mt);
            int j = randN(mt);
            if(i > j) swap(i,j);
            if(i == j || (i == 0 && j == n - 1)) continue;

            int idx_prev_i = ((i - 1 + n) % n);
            int node_prev_i = solution[idx_prev_i];
            int node_i = solution[i];
            int next_j = (j + 1) % n;
            int node_j = solution[j];
            int node_next_j = solution[next_j];

            long long delta = -distMatrix[node_prev_i * n + node_i] 
                              - distMatrix[node_j * n + node_next_j]
                              + distMatrix[node_prev_i * n + node_j] 
                              + distMatrix[node_i * n + node_next_j];

            // Sprawdzamy czy nowe krawędzie które chcemy dodać są zablokowane na liście Tabu
            // Używamy || (OR), bo jeśli choć jedna połówka ruchu jest zablokowana, cały ruch jest podejrzany
            bool isTabu = (tabuList[node_prev_i * n + node_j] > iter) || 
                          (tabuList[node_i * n + node_next_j] > iter);

            // Kryterium Aspiracji (Ignorujemy Tabu, jeśli bijemy globalny rekord)
            if (isTabu && (currCost + delta < globalBestCost)) {
                isTabu = false;
            }

            if (!isTabu && delta < bestDelta) {
                bestDelta = delta;
                newI = i; 
                newJ = j;
            }
        }

        // Zabezpieczenie gdyby przez zbieg okoliczności nie znaleziono żadnego ruchu
        if (newI == -1) break;

        // Wyciągamy wierzchołki starych krawędzi (zrywanych)
        int broken_node_prev_i = solution[((newI - 1 + n) % n)];
        int broken_node_i = solution[newI];
        int broken_node_j = solution[newJ];
        int broken_node_next_j = solution[(newJ + 1) % n];

        // aplikujemy invert
        std::reverse(solution.begin() + newI, solution.begin() + newJ + 1);
        currCost += bestDelta;

        // czy pobiliśmy globalne optimum? jeśli tak aktualizujemy rekord i resetujemy licznik iteracji bez poprawy
        if (currCost < globalBestCost) {
            globalBestCost = currCost;
            bestSolution = solution; // Zapisujemy nowy rekord
            iterWithoutImprovement = 0;
        } else {
            iterWithoutImprovement++;
        }

        // Zapisujemy zrywane krawędzie jako TABU
        tabuList[broken_node_prev_i * n + broken_node_i] = iter + tabuLength;
        tabuList[broken_node_i * n + broken_node_prev_i] = iter + tabuLength;
        
        tabuList[broken_node_j * n + broken_node_next_j] = iter + tabuLength;
        tabuList[broken_node_next_j * n + broken_node_j] = iter + tabuLength;
    }

    solution = bestSolution;
    return globalBestCost;
}