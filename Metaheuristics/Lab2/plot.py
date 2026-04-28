import random
import matplotlib.pyplot as plt
import os

# returns list of tuples (x,y) with coordinates of cities [(1.0, 2.0), (3.0, 4.0), ...]
def readFile(filename):
    coords = []
    cost = 0
    with open(filename) as f:
        # print(f.read())
        cost = int(f.readline())
        for line in f:
            line_split = line.split()
            coords.append((float(line_split[0]), float(line_split[1])))

    return cost, coords


# plots the path from a list of coordinates as a permutations of cities, and saves the plot to a file with the given filename
def plot_path(path, min, filename):
    x = [p[0] for p in path]
    y = [p[1] for p in path]

    x.append(path[0][0])
    y.append(path[0][1])

    plt.figure(figsize=(10,6))
    plt.plot(x, y, 'ro-', markersize=2)
    plt.title(f"Znaleziony cykl ma długość {min}")
    plt.xlabel("X")
    plt.ylabel("Y")
    plt.savefig("plots/" + filename + ".png")

data_dir = os.path.join(os.path.dirname(__file__), 'dataSol')
files = os.listdir(data_dir)
print(f"no. files: {len(files)}\n")

file = "dj38.sol"
for file in files:
    cost, coords = readFile("dataSol/"+file)
    plot_path(coords, cost, file.split(".")[0])
    print(f"{file}")
# best path is [(0.0, 0.0), (1.0, 1.0)], best overall is number and filename is string, so we can use them to plot the path and save it to a file with the name of the algorithm and the filename
#plot_path(best_path_overall, best_overall, filename+algorithm.__name__)