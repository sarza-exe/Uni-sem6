#pragma once

#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <cmath>
#include <climits>
#include <omp.h>

#include "utils.hpp"
#include "localSearch.hpp"

using namespace std;

// Struktura reprezentująca osobnika w populacji
struct Individual {
    vector<int> tour;
    long long cost;
};

// Typ wyliczeniowy do wyboru metody krzyżowania (w celu porównania do sprawozdania)
enum CrossoverType { OX, PMX };


// Implementacja Selekcji Turniejowej
int tournamentSelection(const vector<Individual>& population, int tournamentSize, mt19937& rng) {
    uniform_int_distribution<int> dist(0, population.size() - 1);
    int bestIdx = dist(rng);
    
    for (int i = 1; i < tournamentSize; ++i) {
        int idx = dist(rng);
        if (population[idx].cost < population[bestIdx].cost) {
            bestIdx = idx;
        }
    }
    return bestIdx;
}

// Implementacja Krzyżowania OX (Order Crossover)
Individual crossoverOX(const Individual& p1, const Individual& p2, mt19937& rng) {
    int n = p1.tour.size();
    uniform_int_distribution<int> dist(0, n - 1);
    int idx1 = dist(rng);
    int idx2 = dist(rng);
    if (idx1 > idx2) swap(idx1, idx2);

    Individual child;
    child.tour.assign(n, -1);
    vector<bool> inside(n, false);

    // Kopiowanie fragmentu od pierwszego rodzica
    for (int i = idx1; i <= idx2; ++i) {
        child.tour[i] = p1.tour[i];
        inside[p1.tour[i]] = true;
    }

    // Uzupełnianie reszty z drugiego rodzica (z zachowaniem kolejności i wrap-around)
    int childIdx = (idx2 + 1) % n;
    int parentIdx = (idx2 + 1) % n;

    while (child.tour[childIdx] == -1) {
        if (!inside[p2.tour[parentIdx]]) {
            child.tour[childIdx] = p2.tour[parentIdx];
            childIdx = (childIdx + 1) % n;
        }
        parentIdx = (parentIdx + 1) % n;
    }
    return child;
}

// Implementacja Krzyżowania PMX (Partially Mapped Crossover)
Individual crossoverPMX(const Individual& p1, const Individual& p2, mt19937& rng) {
    int n = p1.tour.size();
    uniform_int_distribution<int> dist(0, n - 1);
    int idx1 = dist(rng);
    int idx2 = dist(rng);
    if (idx1 > idx2) swap(idx1, idx2);

    Individual child;
    child.tour.assign(n, -1);
    
    vector<int> posInP2(n);
    for (int i = 0; i < n; ++i) {
        posInP2[p2.tour[i]] = i;
    }

    // Kopiowanie segmentu z P1
    for (int i = idx1; i <= idx2; ++i) {
        child.tour[i] = p1.tour[i];
    }

    // Mapowanie elementów z P2
    for (int i = idx1; i <= idx2; ++i) {
        int candidate = p2.tour[i];
        bool alreadyPresent = false;
        for (int k = idx1; k <= idx2; ++k) {
            if (child.tour[k] == candidate) {
                alreadyPresent = true;
                break;
            }
        }
        
        if (!alreadyPresent) {
            int currIdx = i;
            int mappedValue = p1.tour[currIdx];
            int targetIdx = posInP2[mappedValue];
            
            while (targetIdx >= idx1 && targetIdx <= idx2) {
                currIdx = targetIdx;
                mappedValue = p1.tour[currIdx];
                targetIdx = posInP2[mappedValue];
            }
            child.tour[targetIdx] = candidate;
        }
    }

    // Przepisanie reszty elementów bezpośrednio z P2
    for (int i = 0; i < n; ++i) {
        if (child.tour[i] == -1) {
            child.tour[i] = p2.tour[i];
        }
    }
    return child;
}

// Implementacja Mutacji przez Odwrócenie (Invert)
void mutateInvert(Individual& ind, double mutationRate, mt19937& rng) {
    uniform_real_distribution<double> rdist(0.0, 1.0);
    if (rdist(rng) < mutationRate) {
        int n = ind.tour.size();
        uniform_int_distribution<int> idist(0, n - 1);
        int i = idist(rng);
        int j = idist(rng);
        if (i > j) swap(i, j);
        if (i != j) {
            reverse(ind.tour.begin() + i, ind.tour.begin() + j + 1);
        }
    }
}

// Migracja między wyspami (Sekwencyjna)
void executeMigration(vector<vector<Individual>>& islands) {
    int numIslands = islands.size();
    vector<vector<Individual>> migrants(numIslands);

    // 1. Pobierz 5 najlepszych z każdej wyspy
    for (int i = 0; i < numIslands; ++i) {
        sort(islands[i].begin(), islands[i].end(), [](const Individual& a, const Individual& b) {
            return a.cost < b.cost;
        });
        for (int k = 0; k < 5; ++k) {
            migrants[i].push_back(islands[i][k]);
        }
    }

    // 2. Prześlij na następną wyspę zastępując 5 najgorszych osobników
    for (int i = 0; i < numIslands; ++i) {
        int nextIsland = (i + 1) % numIslands;
        
        // Sortujemy wyspę docelową tak, aby najgorsi byli na początku (malejąco po koszcie)
        sort(islands[nextIsland].begin(), islands[nextIsland].end(), [](const Individual& a, const Individual& b) {
            return a.cost > b.cost;
        });
        
        for (int k = 0; k < 5; ++k) {
            islands[nextIsland][k] = migrants[i][k];
        }
    }
}

// GŁÓWNY ALGORYTM GENETYCZNY
Individual runIslandGeneticAlgorithm(const vector<int>& distMatrix, int n, CrossoverType crossType) {
    const int NUM_ISLANDS = 8;
    const int POP_SIZE = 100;
    const int TOURNAMENT_SIZE = 5;
    const double MUTATION_RATE = 0.05; 
    
    // PARAMETRY PRZYSPIESZAJĄCE
    const double LS_PROBABILITY = 0.10;      // Tylko 10% dzieci przechodzi Local Search!
    const int STAGNATION_LIMIT = 150;        // 150 pokoleń bez poprawy wystarczy
    
    vector<vector<Individual>> islands(NUM_ISLANDS, vector<Individual>(POP_SIZE));
    vector<mt19937> rngs(NUM_ISLANDS);
    random_device rd;
    for (int i = 0; i < NUM_ISLANDS; ++i) rngs[i].seed(rd() + i);

    // Inicjalizacja populacji początkowej - tutaj warto dać full Local Search, żeby wyspy startowały z wysokiego poziomu
    for (int i = 0; i < NUM_ISLANDS; ++i) {
        for (int j = 0; j < POP_SIZE; ++j) {
            islands[i][j].tour = randomPermutation(n);
            islands[i][j].cost = localSearch(distMatrix, islands[i][j].tour, 10); // Start z lokalnego optimum
        }
    }

    long long globalBestCost = LLONG_MAX;
    Individual globalBestIndividual;
    int generationsWithoutImprovement = 0;
    
    uniform_real_distribution<double> lsidist(0.0, 1.0);

    while (generationsWithoutImprovement < STAGNATION_LIMIT) {
        bool localImprovementFound = false;

        #pragma omp parallel for num_threads(NUM_ISLANDS) shared(islands, rngs)
        for (int islandId = 0; islandId < NUM_ISLANDS; ++islandId) {
            auto& rng = rngs[islandId];
            
            for (int epoch = 0; epoch < 50; ++epoch) {
                vector<Individual> nextGeneration;
                nextGeneration.reserve(POP_SIZE);
                
                auto bestOnIsland = min_element(islands[islandId].begin(), islands[islandId].end(), 
                    [](const Individual& a, const Individual& b) { return a.cost < b.cost; });
                nextGeneration.push_back(*bestOnIsland);

                while (nextGeneration.size() < POP_SIZE) {
                    int p1Idx = tournamentSelection(islands[islandId], TOURNAMENT_SIZE, rng);
                    int p2Idx = tournamentSelection(islands[islandId], TOURNAMENT_SIZE, rng);

                    // powinno się zagwarantować, aby wybrano dwóch różnych rodziców, w celu uniknięcia tworzenia duplikatów
                    while( p1Idx == p2Idx){
                        p2Idx = tournamentSelection(islands[islandId], TOURNAMENT_SIZE, rng);
                    }
                    
                    Individual child;
                    if (crossType == OX) {
                        child = crossoverOX(islands[islandId][p1Idx], islands[islandId][p2Idx], rng);
                    } else {
                        child = crossoverPMX(islands[islandId][p1Idx], islands[islandId][p2Idx], rng);
                    }

                    mutateInvert(child, MUTATION_RATE, rng);

                    // memtyka zoptymalizowana,  LS odpala się losowo, a nie dla każdego!
                    if (lsidist(rng) < LS_PROBABILITY) {
                        child.cost = localSearch(distMatrix, child.tour, 10);
                    } else {
                        child.cost = calculateCost(distMatrix, child.tour); // Zwykłe szybkie liczenie kosztu
                    }

                    nextGeneration.push_back(child);
                }
                islands[islandId] = move(nextGeneration);
            }
        }

        // Kontrola rekordów
        for (int i = 0; i < NUM_ISLANDS; ++i) {
            for (int j = 0; j < POP_SIZE; ++j) {
                // Dla pewności, jeśli najlepszy osobnik nie miał robionego LS, a otarł się o rekord, możemy go "dokształcić" na koniec bloku
                if (islands[i][j].cost < globalBestCost) {
                    localSearch(distMatrix, islands[i][j].tour, 20);
                    islands[i][j].cost = calculateCost(distMatrix, islands[i][j].tour);
                    
                    if (islands[i][j].cost < globalBestCost) {
                        globalBestCost = islands[i][j].cost;
                        globalBestIndividual = islands[i][j];
                        localImprovementFound = true;
                    }
                }
            }
        }

        if (localImprovementFound) {
            generationsWithoutImprovement = 0;
        } else {
            generationsWithoutImprovement += 50;
        }

        if (generationsWithoutImprovement < STAGNATION_LIMIT) {
            executeMigration(islands);
        }
    }

    return globalBestIndividual;
}