#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

int isMatrixRandom;
int size;
int rank;
int* MatrixChunk;
int mSize;
int* displs;
int* sendcounts;
typedef struct { int v1; int v2;} Edge;
int* MST_w;
int* MST;
int minWeight;
FILE *f_matrix;
FILE *f_output;


//Handling MPI_INIT error
void handle_error(int errcode, const char* msg) {
    if (errcode != MPI_SUCCESS) {
        char err_string[1024];
        int len;
        MPI_Error_string(errcode, err_string, &len);
        fprintf(stderr, "Error in %s: %s\n", msg, err_string);
        MPI_Abort(MPI_COMM_WORLD, errcode);
    }
}


//Printing Matrix in console and Output File
void pretty_print_matrix(int* matrix, int rows, int cols, int size, FILE* f_output) {
    if (matrix == NULL) {
        printf("Matrix is NULL\n");
        return;
    }

    char** matrix_str = (char**)malloc(rows * sizeof(char*));
    if (matrix_str == NULL) {
        printf("Memory allocation failed.\n");
        return;
    }

    for (int i = 0; i < rows; ++i) {
        matrix_str[i] = (char*)malloc((cols * 6 + 1) * sizeof(char));
        if (matrix_str[i] == NULL) {
            printf("Memory allocation failed.\n");
            return;
        }
    }

    for (int i = 0; i < rows; ++i) {
        matrix_str[i][0] = '\0';
        for (int j = 0; j < cols; ++j) {
            char buffer[10];
            snprintf(buffer, sizeof(buffer), "%4d ", matrix[i * cols + j]);
            strcat(matrix_str[i], buffer);
        }
    }

    printf("Formatted Matrix:\n");
    if(rows > 10){
        printf("Too big to print in console - written to file");
    }
    fprintf(f_output , "------------------------------------------------\n");
    fprintf(f_output , "Created Matrix for %d ranks: \n", size);
    for (int i = 0; i < rows; ++i) {
        if(rows <= 10){        
            printf("%s\n", matrix_str[i]);
        }
        fprintf(f_output, "%s\n", matrix_str[i]);
        free(matrix_str[i]);
    }
    fprintf(f_output, "\n");
    free(matrix_str);
}

int main(int argc, char *argv[]) {
    
    //Initializing MPI variables
    int errcode = MPI_Init(&argc, &argv);
    handle_error(errcode, "MPI_Init");
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    srand(12345);

    //Process 0 tries to read matrix size from Input file and opens Output file
    if (rank == 0) {
        f_output = fopen("Output/Output.txt", "a");
        if (f_output == NULL) {
            printf("Error in Opening Output File!\n");
        }

        f_matrix = fopen("Input/Matrix.txt", "r");
        if (f_matrix) {
            if (fscanf(f_matrix, "%d", &mSize) != 1){
                mSize = 64;
                isMatrixRandom = 1;
                printf("Matrix.txt not created properly, generating matrix of size %d\n", mSize);
            } else {
                if(mSize <= 2){
                    mSize = 64;
                    isMatrixRandom = 1;
                    printf("Matrix size is too small, generating matrix of size %d\n", mSize);
                } else if(mSize > 2048){
                    mSize = 64;
                    isMatrixRandom = 1;
                    printf("Matrix size is too big, generating matrix of size %d\n", mSize);
                } else{
                    if(fscanf(f_matrix, "%d", &isMatrixRandom) == 1){
                        isMatrixRandom = 0;
                        printf("Matrix.txt created properly, getting matrix of size %d from file\n", mSize);
                    } else {
                        isMatrixRandom = 1;
                        printf("Matrix.txt created properly, generating matrix of size %d\n", mSize);
                    }
                }
            }
        } else {
            mSize = 64;
            isMatrixRandom = 1;
            printf("Matrix.txt not found,  generating matrix of size %d\n", mSize);
            f_matrix = NULL; 
        }
        fclose(f_matrix);
    }

    //Broadcast the matrix size to all processes
    MPI_Bcast(&mSize, 1, MPI_INT, 0, MPI_COMM_WORLD);

    //Allocate memory for displacements and sendcounts
    displs = (int*)malloc(sizeof(int) * size);
    sendcounts = (int*)malloc(sizeof(int) * size);

    //Calculate the base number of rows for each process
    int baseRows = mSize / size;
    int extraRows = mSize % size;
    for (int i = 0; i < size; ++i) {
        sendcounts[i] = baseRows + (i < extraRows ? 1 : 0);
        displs[i] = (i == 0) ? 0 : displs[i - 1] + sendcounts[i - 1];
    }

    //Initialize matrix - if Matrix is in file Matrix.txt it MUST be created properly
    int* matrix = NULL;
    if (rank == 0) {
        matrix = (int*)malloc(mSize * mSize * sizeof(int));
        if(isMatrixRandom) {
            for (int i=0; i < mSize; ++i) {
                matrix[mSize * i + i] = 0;
            }
            for (int i = 0; i < mSize; ++i) {
                for (int j = i + 1; j < mSize; ++j) {
                    int weight;
                    if(matrix[mSize * i + j - 1] == 0) {
                        weight = rand() % 20 + 1;
                    } else {
                        weight = rand() % 20;
                        if (rand() % 4 == 0) {
                            weight = 0;
                        }
                    }
                    matrix[mSize * i + j] = weight;
                    matrix[mSize * j + i] = weight;
                }
            }
        } else {
            f_matrix = fopen("Input/Matrix.txt", "r");
            fscanf(f_matrix, "%d", &isMatrixRandom);
            for (int i = 0; i < mSize; ++i) {
                for (int j = 0; j < mSize; ++j) {
                    fscanf(f_matrix, "%d", &matrix[mSize * i + j]);
                }
            }
            fclose(f_matrix);
        }
    }

    //Print the matrix on rank 0 (before scattering)
    if (rank == 0) {
        printf("Input Matrix (Rank 0):\n");
        pretty_print_matrix(matrix, mSize, mSize, size, f_output);
        fclose(f_output);
    }

    //Allocate memory for the matrix chunk for each process
    MatrixChunk = (int*)malloc(sendcounts[rank] * mSize * sizeof(int));

    //Create MPI data type for matrix row
    MPI_Datatype matrixString;
    MPI_Type_contiguous(mSize, MPI_INT, &matrixString);
    MPI_Type_commit(&matrixString);

    //Scatter matrix to all processes
    errcode = MPI_Scatterv(matrix, sendcounts, displs, matrixString, MatrixChunk, sendcounts[rank], matrixString, 0, MPI_COMM_WORLD);
    handle_error(errcode, "MPI_Scatterv");

    //Free the custom datatype after usage
    MPI_Type_free(&matrixString);

    //Free the full matrix on rank 0 after scattering
    if (rank == 0) free(matrix);

    //Initialize MST array and other variables
    MST = (int*)malloc(sizeof(int) * mSize);
    MST_w = (int*)malloc(sizeof(int) * mSize);
    for (int i = 0; i < mSize; ++i) {
        MST[i] = -1;
        MST_w[i] = 0;
    }
    
    double start = MPI_Wtime();
    MST[0] = 0;
    minWeight = 0;

    struct { int min; int rank; } minRow, row;
    Edge edge;

    //Perform Prim's algorithm to find MST
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
        MST_w[edge.v2] = minRow.min;
        minWeight += minRow.min;
    }

    double finish = MPI_Wtime();
    double calc_time = finish - start;

    //Rank 0 writes the result to a file and prints it
    if (rank == 0) {
        f_output = fopen("Output/Output.txt", "a");
        if (f_output == NULL) {
            printf("Error in Opening Output File!\n");
        } else {

            fprintf(f_output, "The min weight is %d\n", minWeight);
            for (int i = 0; i < mSize; ++i) fprintf(f_output, "Edge %d %d - weight %d \n", i, MST[i], MST_w[i]);
            fprintf(f_output, "\nProcessors: %d\nVertices: %d\nExecution Time: %f\nTotal Weight: %d\n\n", size, mSize, calc_time, minWeight);


            fclose(f_output);
        }
        
        printf("\nMST Result (Rank 0):\n");
        for (int i = 0; i < mSize; ++i) {
            printf("Vertex %d -> Parent %d - Weight: %d\n", i, MST[i], MST_w[i]);
        }
        printf("Total Weight: %d\n", minWeight);
        printf("\nExecution Time: %f seconds\n", calc_time);
    }

    free(MatrixChunk);
    free(MST);
    free(sendcounts);
    free(displs);

    MPI_Finalize();
    return 0;
}
