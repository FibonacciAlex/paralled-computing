#define _CRT_SECURE_NO_WARNINGS
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MASTER 0
#define  A 1664525
#define  C 1013904223
#define  M 4294967296UL // 2^32
#define MAX_LINE_LENGTH 100
#define MAX_ENTRIES 1000

typedef unsigned long ULONG;

#if 1

typedef struct {
    int k;
    unsigned long k_a;
    unsigned long k_c;
    int flag;
} JumpConstant;

// Function to read file and populate the array of JumpConstant
int readFile(const char* file_path, JumpConstant jumpConstants[], int* count) {
    FILE* file = fopen(file_path, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Could not open the file.\n");
        return 1;
    }

    char line[MAX_LINE_LENGTH];



    // Read the file line by line
    *count = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        if (sscanf(line, "%d %lu %lu %d", &jumpConstants[*count].k, &jumpConstants[*count].k_a, &jumpConstants[*count].k_c, &jumpConstants[*count].flag) == 4) {
            (*count)++;
            if (*count >= MAX_ENTRIES) {
                fprintf(stderr, "Warning: Maximum number of entries reached.\n");
                break;
            }
        }
        else {
            fprintf(stderr, "Error: Could not parse line: %s", line);
        }
    }

    fclose(file);
    return 0;
}

// Evaluate (ax+c) mod m
ULONG modlin(ULONG a, ULONG x, ULONG c, unsigned long long m)
{
    ULONG i_r = (a * x + c) % m;
    return i_r;
}

// Put integer n in range x1 , x2 with the maximum integer value
double rescale(ULONG N, ULONG n, double x1, double x2)
{
    double f = static_cast<double>(n) / static_cast<double>(N);
    return x1 + f * (x2 - x1);
}

int main(int argc, char* argv[]) {
    int num_tasks, task_id, num_points,i_server, points_in_circle = 0;
    int total_points_in_circle;
    double x, y, pi_estimate;
    double start_time, end_time;
    unsigned long* rand_nums;
    ULONG i_next=0,i_random = 0;
    ULONG seed = 12345;// Seed value
    ULONG a, c;
    const ULONG sidelen = 65536; // sqrt of m
   

    // Initialize the MPI environment
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &task_id);
    MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);
    printf("Number of My rank= %d\n and tasks= %d\n ", task_id,num_tasks);

    //argv[0] is project name.
    if (argc != 2) {
        if (task_id == MASTER) {
            printf("Usage: %s <number of points per process>\n", argv[0]);
        }
        MPI_Finalize();
        exit(1);
    }
    //argv[1] is the number of points.
    num_points = atoi(argv[1]);   //调用方法运行时录入要运行的点的个数

    if (task_id == MASTER) {
        // Master process generates random numbers for all processes
        rand_nums = (unsigned long*)malloc(num_points * num_tasks * sizeof(unsigned long)); 
      
        for (ULONG n = 0; n < num_points ; ++n) {
            rand_nums[n] = seed;
            i_next = modlin(A, seed, C, M);
            seed = i_next;
            // Scale the random number to a random 2−d position
            i_random = i_next;
            ULONG ix = i_random % sidelen;
            ULONG iy = i_random / sidelen;
            // Scale current random integer to value from 0−1
            x = rescale(sidelen, ix, -1, 1);
            y = rescale(sidelen, iy, -1, 1);
            // Now we have an (x , y) pair generated from a single random integer

       //     printf("master process gets value of x = %lf ,y = %lf\n ", x, y);
            if (x * x + y * y <= 1.0) {
                points_in_circle++;
            }
        }

        // Start the timer
        start_time = MPI_Wtime();

        JumpConstant jumpConstants[MAX_ENTRIES];
        int count = 0;
        // Specify the path to the file
        const char* file_path = "jumpconstants.dat";
        // Call the readFile function
        if (readFile(file_path, jumpConstants, &count) != 0) {
            fprintf(stderr, "Error: Could not read the file properly.\n");
            return 1;
        }
        // Example usage: Print all the read data
        for (int i = 0; i < count; i++) {
            if (jumpConstants[i].k == num_tasks) {
         //       printf("k: %d, A: %lu, C: %lu, flag: %d\n", jumpConstants[i].k, jumpConstants[i].k_a, jumpConstants[i].k_c, jumpConstants[i].flag);
                a = jumpConstants[i].k_a;
                c = jumpConstants[i].k_c;
            }
        }
        
        // Via leapfrog to distribute random numbers to  process
        for (int i = 1; i < num_tasks; i++) {
           
            printf("Request %d is assigned to server %d\n", i, i);
            MPI_Send(&rand_nums[i * num_points], num_points, MPI_UNSIGNED_LONG, i, 0, MPI_COMM_WORLD);
            MPI_Recv(&rand_nums[i * num_points], num_points, MPI_UNSIGNED_LONG, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        

        // Master process also uses its portion of the random numbers
       
    }
    else {
        // Other processes receive their random numbers
        rand_nums = (unsigned long*)malloc(num_points * sizeof(unsigned long));
        std::cout<<"task_id wait ["<<task_id<<"]"<<std::endl;
        
        MPI_Recv(rand_nums, num_points, MPI_UNSIGNED_LONG, MASTER, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        std::cout<<"task_id recv ["<<task_id<<"]"<<std::endl;

        MPI_Send(rand_nums, num_points, MPI_UNSIGNED_LONG, MASTER, 0, MPI_COMM_WORLD);
        std::cout<<"task_id send ["<<task_id<<"]"<<std::endl;

        // Each process performs its computation
        for (int i = 0; i < num_points; i++) {
            rand_nums[i] = seed;
            i_next = modlin(A, seed, C, M);
            seed = i_next;
            // Scale the random number to a random 2−d position
            i_random = i_next;
            ULONG ix = i_random % sidelen;
            ULONG iy = i_random / sidelen;
            // Scale current random integer to value from 0−1
             x = rescale(sidelen, ix, -1, 1);
             y = rescale(sidelen, iy, -1, 1);
            // Now we have an (x , y) pair generated from a single random integer

          //  printf("slave process get value of x = %lf ,y = %lf\n ", x, y);
            if (x * x + y * y <= 1.0) {
                points_in_circle++;
            }
        }
    }

    // Reduce the results to get the total number of points in the circle
    MPI_Reduce(&points_in_circle, &total_points_in_circle, 1, MPI_INT, MPI_SUM, MASTER, MPI_COMM_WORLD);

    // Stop the timer
    if (task_id == MASTER) {
        end_time = MPI_Wtime();

        // Calculate and print the value of π
        pi_estimate = 4.0 * total_points_in_circle / (num_points * num_tasks);
        printf("Estimated value of Pi: %f\n", pi_estimate);
        printf("Total number of points: %d\n", num_points * num_tasks);
        printf("Total points in circle: %d\n", total_points_in_circle);
        printf("Execution time: %f seconds\n", end_time - start_time);
    }

    // Free allocated memory
    free(rand_nums);

    // Finalize the MPI environment
    MPI_Finalize();

    return 0;
}
#endif