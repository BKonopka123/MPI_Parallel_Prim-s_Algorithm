#include <upcxx/upcxx.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <limits>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <iomanip>

constexpr int INF = std::numeric_limits<int>::max();

int main(int argc, char** argv) {
    //Initialization
    upcxx::init();

    int rank = upcxx::rank_me();
    int ranks = upcxx::rank_n();
    std::srand(12345);

    std::string input_file = "Input/Matrix.txt";
    std::ifstream infile(input_file);
    if (!infile.is_open()) {
        if (rank == 0) {
            std::cerr << "Failed to open input file: " << input_file << std::endl;
        }
        upcxx::finalize();
        return 1;
    }

    int mSize;
    infile >> mSize;
    std::vector<int> matrix(mSize * mSize, 0);
    
    int test_val;
    if (infile >> test_val) {
        matrix[0] = test_val;
        for (int i = 1; i < mSize * mSize; ++i) {
            infile >> matrix[i];
        }
    } else {
        for (int i = 0; i < mSize; ++i) {
            for (int j = i + 1; j < mSize; ++j) {
                int weight;
                if(matrix[mSize * i + j - 1] == 0) {
                    weight = std::rand() % 20 + 1;
                } else {
                    weight = std::rand() % 20;
                    if (std::rand() % 4 == 0) {
                        weight = 0;
                    }
                }
                matrix[i * mSize + j] = weight;
                matrix[j * mSize + i] = weight;
            }
        }
    }
    infile.close();

    //Sending data to ranks and calculating MST
    int vertices_per_rank = (mSize + ranks - 1) / ranks;
    int start = rank * vertices_per_rank;
    int end = std::min(mSize, start + vertices_per_rank);

    std::vector<bool> selected(mSize, false);
    selected[0] = true;

    int edge_count = 0;
    int total_weight = 0;
    std::vector<std::tuple<int, int, int>> mst_edges;

    upcxx::global_ptr<int> g_matrix = upcxx::new_array<int>(mSize * mSize);
    upcxx::rput(matrix.data(), g_matrix, mSize * mSize).wait();
    upcxx::barrier();
    auto start_time = std::chrono::high_resolution_clock::now();

    while (edge_count < mSize - 1) {
        int local_min_weight = INF;
        int local_u = -1, local_v = -1;

        for (int i = 0; i < mSize; ++i) {
            if (selected[i]) {
                for (int j = start; j < end; ++j) {
                    if (!selected[j]) {
                        int weight = upcxx::rget(g_matrix + i * mSize + j).wait();
                        if (weight != 0 && weight < local_min_weight) {
                            local_min_weight = weight;
                            local_u = i;
                            local_v = j;
                        }
                    }
                }
            }
        }
        std::pair<int, int> local_data{local_min_weight, rank};
        std::pair<int, int> global_data;

        auto cmp_min = [](const std::pair<int,int> &a, const std::pair<int,int> &b) {
            if (a.first != b.first)
                return (a.first < b.first) ? a : b;
            else
                return (a.second < b.second) ? a : b;
        };
        
        upcxx::reduce_all(&local_data, &global_data, 1, cmp_min, upcxx::world()).wait();

        int chosen_u = -1, chosen_v = -1;
        int chosen_weight = INF;

        if (rank == global_data.second) {
            chosen_u = local_u;
            chosen_v = local_v;
            chosen_weight = local_min_weight;
        }

        chosen_u = upcxx::broadcast(chosen_u, global_data.second).wait();
        chosen_v = upcxx::broadcast(chosen_v, global_data.second).wait();
        chosen_weight = upcxx::broadcast(chosen_weight, global_data.second).wait();

        selected[chosen_v] = true;
        total_weight += chosen_weight;
        mst_edges.emplace_back(chosen_u, chosen_v, chosen_weight);
        ++edge_count;

        upcxx::barrier();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> exec_time = end_time - start_time;

    //Saving data to file
    if (rank == 0) {
        std::ofstream outfile("Output/Output.txt", std::ios::app);
        outfile << "------------------------------------------------\n";
        outfile << "Created Matrix for " << ranks << " ranks:\n";
        for (int i = 0; i < mSize; ++i) {
            for (int j = 0; j < mSize; ++j) {
                outfile << std::setw(4) << matrix[i * mSize + j];
            }
            outfile << "\n";
        }
        outfile << "\nThe min weight is " << total_weight << "\n";
        for (const auto& [u, v, w] : mst_edges) {
            outfile << "Edge " << v << " " << u << " - weight " << w << "\n";
        }
        outfile << "\nProcessors: " << ranks << "\n";
        outfile << "Vertices: " << mSize << "\n";
        outfile << "Execution Time: " << std::fixed << std::setprecision(8) << exec_time.count() << " s\n";
        outfile << "Total Weight: " << total_weight << "\n";
        outfile.close();
    }

    upcxx::delete_array(g_matrix);
    upcxx::finalize();
    return 0;
}
