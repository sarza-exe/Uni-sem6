import random
import math
import matplotlib.pyplot as plt
import os

# the cost of travel between cities is specified by the Euclidean distance rounded to the nearest whole number
def EUC_2Dnorm(a, b):
    return int(math.sqrt((b[0]-a[0])**2 + (b[1]-a[1])**2)+0.5)

def hamiltonianLength(candidates):
    length = EUC_2Dnorm(candidates[len(candidates)-1], candidates[0])
    for i in range(len(candidates)-1):
        length += EUC_2Dnorm(candidates[i], candidates[i+1])
    return length

# returns list of tuples (x,y) with coordinates of cities [(1.0, 2.0), (3.0, 4.0), ...]
def readFile(filename):
    coords = []
    with open(filename) as f:
        # print(f.read())
        for line in f:
            line_split = line.split()
            if(line_split[0].isdigit()):
                #print(line, end="")
                coords.append((float(line_split[1]), float(line_split[2])))

    return coords

"""
Perform local search optimization on TSP coordinates (all neighbors).

Args:
    coords (list[tuple[float, float]]): List of (x, y) coordinates for cities.

Returns:
    min_total: cost of the best solution found.
    no_steps: number of iterations taken to reach the local optimum.
    coords: the best solution found (permutation of cities).
"""
def localSearch(coords):
    if not coords:
        return 0, 0, []
    
    min_total = hamiltonianLength(coords)
    no_steps = 0

    while True:
        no_steps += 1
        improved = False
        for i in range(len(coords)-1):
            for j in range(i+1, len(coords)):
                new_coords = coords.copy()
                new_coords[i], new_coords[j] = new_coords[j], new_coords[i]
                new_length = hamiltonianLength(new_coords)
                if new_length < min_total:
                    coords = new_coords
                    min_total = new_length
                    improved = True
        if not improved:
            break

    return min_total, no_steps, coords

"""
Perform local search optimization on TSP coordinates for n random neighbors.

Args:
    coords (list[tuple[float, float]]): List of (x, y) coordinates for cities.

Returns:
    min_total: cost of the best solution found.
    no_steps: number of iterations taken to reach the local optimum.
    coords: the best solution found (permutation of cities).
"""
def localSearchFast(coords):
    if not coords:
        return 0, 0, []
    
    min_total = hamiltonianLength(coords)
    no_steps = 0

    while True:
        no_steps += 1
        improved = False
        invert_coords = getInvertCoords(len(coords))
        for (i,j) in invert_coords:
            new_coords = coords.copy()
            new_coords[i], new_coords[j] = new_coords[j], new_coords[i]
            new_length = hamiltonianLength(new_coords)
            if new_length < min_total:
                    coords = new_coords
                    min_total = new_length
                    improved = True
        if not improved:
            break

    return min_total, no_steps, coords


def getInvertCoords(n):
    invert_coords = []
    for i in range(n-1):
        for j in range(i+1, n):
            invert_coords.append((i, j))
    random.shuffle(invert_coords)
    return invert_coords[:n]


def prim_MST(coords):
    n = len(coords)
    tree = {i: [] for i in range(n)} # np. tree[0] = [1, 5] means city 0 is connected with cities 1 and 5
    in_tree = [False] * n
    
    min_dist = [float('inf')] * n # min distance from city i to the tree
    parent = [-1] * n

    min_dist[0] = 0
    weight = 0
    
    for _ in range(n):
        # find vertix with min distance from the tree
        best_dist = float('inf')
        u = -1
        for i in range(n):
            if not in_tree[i] and min_dist[i] < best_dist:
                best_dist = min_dist[i]
                u = i
                
        in_tree[u] = True # add u to the tree
        
        # if not first, add edge to the tree
        if parent[u] != -1:
            tree[u].append(parent[u])
            tree[parent[u]].append(u)
            weight += best_dist
            
        # update min_dist for all vertices not in the tree
        for v in range(n):
            if not in_tree[v]:
                dist = EUC_2Dnorm(coords[u], coords[v])
                if dist < min_dist[v]:
                    min_dist[v] = dist
                    parent[v] = u


    return tree, weight

def dfs_path(tree, coords, start):
    n = len(coords)
    mark = [False]*n
    path = []
    stack = [start]
    while(len(stack) != 0):
        v = stack.pop()
        mark[v] = True
        path.append(coords[v])
        for u in tree[v]:
            if not mark[u]:
                stack.append(u)
    return path

# plots the path from a list of coordinates as a permutations of cities, and saves the plot to a file with the given filename
def plot_path(path, min, filename):
    x = [p[0] for p in path]
    y = [p[1] for p in path]

    x.append(path[0][0])
    y.append(path[0][1])

    plt.figure(figsize=(10,6))
    plt.plot(x, y, 'ro-')
    plt.title(f"Znaleziony cykl ma długość {min}")
    plt.xlabel("X")
    plt.ylabel("Y")
    plt.savefig("plots/" + filename + ".png")

def runExperiment(coords, algorithm, filename, num_runs=0):
    min_totals = []
    no_steps_list = []
    best_overall = float('inf')
    best_path_overall = []
    if num_runs <= 0:
        num_runs = len(coords)
    for _ in range(num_runs): # range(min(100, len(coords))) # 4663 is some kind of joke
        shuffled_coords = coords.copy()
        random.shuffle(shuffled_coords)
        min_local, no_steps, coords_local = localSearch(shuffled_coords)
        min_totals.append(min_local)
        no_steps_list.append(no_steps)
        if min_local < best_overall:
            best_overall = min_local
            best_path_overall = coords_local
    avg_min_total = sum(min_totals) / len(min_totals)
    avg_no_steps = sum(no_steps_list) / len(no_steps_list)

    plot_path(best_path_overall, best_overall, filename+algorithm.__name__)

    print(algorithm.__name__)
    print(f"Average solution value: {avg_min_total}")
    print(f"Average number of improvement steps: {avg_no_steps}")
    print(f"Best solution value: {best_overall}")
    print()

def runMstExperiment(coords, tree, filename):
    min_totals = []
    no_steps_list = []
    best_overall = float('inf')
    best_path_overall = []
    num_runs = int(math.sqrt(len(coords)))
    for _ in range(num_runs): # range(min(100, len(coords))) # 4663 is some kind of joke
        random_vertix = random.randint(0, len(coords)-1)
        path = dfs_path(tree, coords, random_vertix)
        min_local, no_steps, coords_local = localSearch(path.copy())
        min_totals.append(min_local)
        no_steps_list.append(no_steps)
        if min_local < best_overall:
            best_overall = min_local
            best_path_overall = coords_local
    avg_min_total = sum(min_totals) / len(min_totals)
    avg_no_steps = sum(no_steps_list) / len(no_steps_list)

    plot_path(best_path_overall, best_overall, filename+"MST_LS")

    print("MST local search")
    print(f"Average solution value: {avg_min_total}")
    print(f"Average number of improvement steps: {avg_no_steps}")
    print(f"Best solution value: {best_overall}")
    print()

data_dir = os.path.join(os.path.dirname(__file__), 'data')
files = os.listdir(data_dir)
print(f"no. files: {len(files)}\n")

file = "dj38.tsp"
# for file in files:
coords = readFile("data/"+file)
print(f"{file}, no. cities {len(coords)}")
runExperiment(coords, localSearch, file.split('.')[0])
runExperiment(coords, localSearchFast, file.split('.')[0])
tree, weight = prim_MST(coords)
print(f"Prim MST weight: {weight}")
runMstExperiment(coords, tree, file.split('.')[0])