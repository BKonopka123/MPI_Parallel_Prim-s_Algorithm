#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <string.h>

int nomatrix;
int size;
int rank;
int* MatrixChunk;
int mSize;
int* displs;
int* sendcounts;
typedef struct { int v1; int v2; } Edge;
int* MST;
int minWeight;
FILE *f_matrix;
FILE *f_time;
FILE *f_result;

void handle_error(int errcode, const char* msg) {
    if (errcode != MPI_SUCCESS) {
        char err_string[1024];
        int len;
        MPI_Error_string(errcode, err_string, &len);
        fprintf(stderr, "Error in %s: %s\n", msg, err_string);
        MPI_Abort(MPI_COMM_WORLD, errcode);
    }
}

void pretty_print_matrix(int* matrix, int rows, int cols) {
    if (matrix == NULL) {
        printf("Matrix is NULL\n");
        return;
    }

    // Allocate an array of strings to hold each row as a string
    char** matrix_str = (char**)malloc(rows * sizeof(char*));
    if (matrix_str == NULL) {
        printf("Memory allocation failed.\n");
        return;
    }

    // Allocate memory for each row's string (with enough space for formatting)
    for (int i = 0; i < rows; ++i) {
        matrix_str[i] = (char*)malloc((cols * 6 + 1) * sizeof(char)); // Each number can take up to 5 chars + 1 for '\0'
        if (matrix_str[i] == NULL) {
            printf("Memory allocation failed.\n");
            return;
        }
    }

    // Format each row and store it in the array of strings
    for (int i = 0; i < rows; ++i) {
        matrix_str[i][0] = '\0'; // Start with an empty string
        for (int j = 0; j < cols; ++j) {
            char buffer[10];
            snprintf(buffer, sizeof(buffer), "%4d ", matrix[i * cols + j]);
            strcat(matrix_str[i], buffer); // Append formatted number to row
        }
    }

    // Print the matrix
    printf("Formatted Matrix:\n");
    for (int i = 0; i < rows; ++i) {
        printf("%s\n", matrix_str[i]);
        free(matrix_str[i]); // Free each row after printing
    }

    free(matrix_str); // Free the array of strings
}

int main(int argc, char *argv[]) {
    int errcode = MPI_Init(&argc, &argv);
    handle_error(errcode, "MPI_Init");

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Generate a seed based on rank and time, ensuring consistency
    unsigned int seed = 12345 + rank;  // Base seed value combined with rank
    srand(seed);

    if (rank == 0) {
        // Request mSize from the user only on rank 0
        printf("Please enter the size of the matrix (mSize): ");
        scanf("%d", &mSize);
        
        f_matrix = fopen("Matrix.txt", "r");
        if (f_matrix) {
            fscanf(f_matrix, "%d\n", &mSize);
        } else {
            // If file doesn't exist, generate random matrix
            printf("Matrix.txt not found, generating random matrix...\n");
            f_matrix = NULL;  // No file, so set the file pointer to NULL
        }
        nomatrix = (f_matrix == NULL) ? 1 : 0;  // If no matrix file, set flag to generate random matrix
    }

    // Broadcast the matrix size to all processes
    MPI_Bcast(&mSize, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Allocate memory for displacements and sendcounts
    displs = (int*)malloc(sizeof(int) * size);
    sendcounts = (int*)malloc(sizeof(int) * size);

    // Calculate the base number of rows for each process
    int baseRows = mSize / size;
    int extraRows = mSize % size;
    for (int i = 0; i < size; ++i) {
        sendcounts[i] = baseRows + (i < extraRows ? 1 : 0);
        displs[i] = (i == 0) ? 0 : displs[i - 1] + sendcounts[i - 1];
    }

    // Initialize matrix
    int* matrix = NULL;
    if (rank == 0) {
        matrix = (int*)malloc(mSize * mSize * sizeof(int));
        if (f_matrix) {
            // If the file is available, read the matrix from it
            for (int i = 0; i < mSize; ++i) {
                for (int j = 0; j < mSize; ++j) {
                    if (i == j) {
                        matrix[mSize * i + j] = 0;  // Diagonal elements are 0
                    } else {
                        fscanf(f_matrix, "%d", &matrix[mSize * i + j]);  // Read matrix values
                    }
                }
            }
            fclose(f_matrix);
        } else {
            // If no matrix file, generate random values for the matrix
            for (int i = 0; i < mSize; ++i) {
                for (int j = i + 1; j < mSize; ++j) {  // Generate only upper triangle for symmetry
                    int weight = rand() % 20 + 1;  // Random weight between 1 and 20
                    matrix[mSize * i + j] = weight;
                    matrix[mSize * j + i] = weight;  // Symmetric edge weight
                }
            }
            for (int i=0; i < mSize; ++i) {
                matrix[mSize * i + i] = 0;
            }
        }
    }

    // Print the matrix on rank 0 (before scattering)
    if (rank == 0) {
        printf("Input Matrix (Rank 0):\n");
        pretty_print_matrix(matrix, mSize, mSize);
    }

    // Allocate memory for the matrix chunk for each process
    MatrixChunk = (int*)malloc(sendcounts[rank] * mSize * sizeof(int));

    // Create MPI data type for matrix row
    MPI_Datatype matrixString;
    MPI_Type_contiguous(mSize, MPI_INT, &matrixString);
    MPI_Type_commit(&matrixString);

    // Scatter matrix to all processes
    errcode = MPI_Scatterv(matrix, sendcounts, displs, matrixString, MatrixChunk, sendcounts[rank], matrixString, 0, MPI_COMM_WORLD);
    handle_error(errcode, "MPI_Scatterv");

    // Free the custom datatype after usage
    MPI_Type_free(&matrixString);

    // Free the full matrix on rank 0 after scattering
    if (rank == 0) free(matrix);

    // Initialize MST array and other variables
    MST = (int*)malloc(sizeof(int) * mSize);
    for (int i = 0; i < mSize; ++i) MST[i] = -1;
    
    double start = MPI_Wtime();
    MST[0] = 0;
    minWeight = 0;

    struct { int min; int rank; } minRow, row;
    Edge edge;

    // Perform Prim's algorithm to find MST
    for (int k = 0; k < mSize - 1; ++k) {
        int min = INT_MAX, v1 = -1, v2 = -1;
        for (int i = 0; i < sendcounts[rank]; ++i) {
            if (MST[i + displs[rank]] != -1) {
                for (int j = 0; j < mSize; ++j) {
                    if (MST[j] == -1 && MatrixChunk[mSize * i + j] < min && MatrixChunk[mSize * i + j] != 0) {
                        min = MatrixChunk[mSize * i + j];
                        v1 = i + displs[rank];
                        v2 = j;
                    }
                }
            }
        }
        row.min = min;
        row.rank = rank;
        MPI_Allreduce(&row, &minRow, 1, MPI_2INT, MPI_MINLOC, MPI_COMM_WORLD);
        
        edge.v1 = v1;
        edge.v2 = v2;
        MPI_Bcast(&edge, 1, MPI_2INT, minRow.rank, MPI_COMM_WORLD);
        
        MST[edge.v2] = edge.v1;
        minWeight += minRow.min;
    }

    double finish = MPI_Wtime();
    double calc_time = finish - start;

    // Rank 0 writes the result to a file and prints it
    if (rank == 0) {
        f_result = fopen("Result.txt", "w");
        fprintf(f_result, "The min weight is %d\n", minWeight);
        for (int i = 0; i < mSize; ++i) fprintf(f_result, "Edge %d %d\n", i, MST[i]);
        fclose(f_result);
        
        f_time = fopen("Time.txt", "a+");
        fprintf(f_time, "\nProcessors: %d\nVertices: %d\nExecution Time: %f\nTotal Weight: %d\n\n", size, mSize, calc_time, minWeight);
        fclose(f_time);
        
        printf("\nMST Result (Rank 0):\n");
        for (int i = 0; i < mSize; ++i) {
            printf("Vertex %d -> Parent %d\n", i, MST[i]);
        }
        printf("Total Weight: %d\n", minWeight);
        printf("\nExecution Time: %f seconds\n", calc_time);
    }

    // Free allocated memory
    free(MatrixChunk);
    free(MST);
    free(sendcounts);
    free(displs);

    MPI_Finalize();
    return 0;
}
