// Code written by Hammad Ather on Jan 23rd, 2022. 

#include <iostream>
#include <math.h>  
#include <mpi.h>

#define MASTER		0
#define BEGIN       1  

using namespace std;

void convergence(float * worker_region, float * worker_region_copy, int timestamp_index, int taskid, int region_size_row, 
                int region_size_col, bool &convergence_flag, float C){
    
    bool change = true;
    for(int i = 0; i < region_size_row; i++){
        for(int j = 0; j < region_size_col; j++){
            if(abs(*((worker_region + i * region_size_col) + j) - *((worker_region_copy + i * region_size_col) + j)) > C){
                change = false;
                break;
            }
        }
        if (change == false)
            break;
    }

    if (change == true) {
        convergence_flag = true;
        cout << "\nTask ID " << taskid << " has converged!\n";
    }
    else {
        if ((timestamp_index + 1) % 5 == 0){
            float max = 0.0;
            for(int i = 0; i < region_size_row; i++){
                for(int j = 0; j < region_size_col; j++){
                    if(abs(*((worker_region + i * region_size_col) + j) - *((worker_region_copy + i * region_size_col) + j)) > max){
                        max = abs(*((worker_region + i * region_size_col) + j) - *((worker_region_copy + i * region_size_col) + j));
                    }
                }
            }   
            cout << "\nTask ID: " << taskid << " maximum update difference = " << max << "\n"; 
        }
    }
}
   

void update(float * worker_region, float * col_left, float *col_right, float *row_below, float *row_above, int left, int right, int above, 
            int below, int timestamp_index, int taskID, int region_size_row, int region_size_col, bool &convergence_flag, float C){
    
    float * worker_region_copy = new float[region_size_col * region_size_row];
    for (int i = 0; i < region_size_row; i++){
        for (int j = 0; j < region_size_col; j++){
            *((worker_region_copy + i * region_size_col) + j) = *((worker_region + i * region_size_col) + j);
        }
    }

    int row_start = 0, row_end = region_size_row, col_start = 0, col_end = region_size_col;
    if (above == -1){
        row_start = 1;
        row_end = region_size_row;
    }
    if (below == -1){
        row_start = 0;
        row_end = region_size_row - 1;
    }
    if (left == -1){
        col_start = 1;
        col_end = region_size_col;
    }
    if (right == -1){
        col_start = 0;
        col_end = region_size_col - 1;
    }

    float left_val = 0.0, right_val = 0.0 , above_val = 0.0, below_val = 0.0;
    for (int i = row_start; i < row_end; i++){
        for(int j = col_start; j < col_end; j++){
            float val = *((worker_region_copy + i * region_size_col) + j);
            if ((j + 1) >= region_size_col){
                right_val = *(col_right + i);
            }else{
                right_val = *((worker_region_copy + i * region_size_col) + (j + 1));
            }
            if ((j - 1) < 0){
                left_val = *(col_left + i);
            }else{
                left_val = *((worker_region_copy + i * region_size_col) + (j - 1));
            }
            if ((i + 1) >= region_size_row){
                below_val = *(row_below + j);
            }else{
                below_val = *((worker_region_copy + (i + 1) * region_size_col) + j);
            }
            if ((i - 1) < 0){
                above_val = *(row_above + j);
            }else{
                above_val = *((worker_region_copy + (i - 1) * region_size_col) + j);
            }
            float new_val = (right_val + left_val + above_val + below_val)/4;
            *((worker_region + i * region_size_col) + j) = new_val;
        }
    }
    convergence(worker_region, worker_region_copy, timestamp_index, taskID, region_size_row, region_size_col, convergence_flag, C);
}


void send(int left, int right, int below, int above, float * worker_region, float * col_left, float * col_right, float * row_below, 
            float * row_above, int region_size_row, int region_size_col, MPI_Status status){
    
    float * col = new float[region_size_row];
    float * row = new float[region_size_col];
    MPI_Request request;

    if(left != -1){
        for (int k = 0; k < region_size_row; k++){
            col[k] = *((worker_region + k * region_size_row) + 0);
        }
        
        MPI_Isend(col, region_size_row, MPI_FLOAT, left, BEGIN, MPI_COMM_WORLD, &request);
        MPI_Irecv(col_left, region_size_row, MPI_FLOAT, left, BEGIN, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, &status);
    }
    if(right != -1){
        for (int k = 0; k < region_size_row; k++){
            col[k] = *((worker_region + k * region_size_row) + (region_size_col - 1));
        }
        MPI_Isend(col, region_size_row, MPI_FLOAT, right, BEGIN, MPI_COMM_WORLD, &request);
        MPI_Irecv(col_right, region_size_row, MPI_FLOAT, right, BEGIN, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, &status);
    }
    if(below != -1){
        for (int k = 0; k < region_size_col; k++){
            row[k] = *((worker_region + (region_size_row - 1) * region_size_col) + k);
        }
        MPI_Isend(row, region_size_col, MPI_FLOAT, below, BEGIN, MPI_COMM_WORLD, &request);
        MPI_Irecv(row_below, region_size_col, MPI_FLOAT, below, BEGIN, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, &status);
    }
    if(above != -1){
        for (int k = 0; k < region_size_col; k++){
            row[k] = *((worker_region + 0 * region_size_col) + k);
        }
        MPI_Isend(row, region_size_col, MPI_FLOAT, above, BEGIN, MPI_COMM_WORLD, &request);
        MPI_Irecv(row_above, region_size_col, MPI_FLOAT, above, BEGIN, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, &status);
    }
}


int main(int argc, char *argv[]){
    
    MPI_Init(&argc, &argv);
    int number_of_processes, taskid;
    MPI_Comm_size(MPI_COMM_WORLD, &number_of_processes);
    MPI_Comm_rank(MPI_COMM_WORLD,&taskid);
    MPI_Status status;

    int M = 1024, N = 2048;
    int timestamps = 100;
    float C = 0.004;

    int region_size_row, region_size_col;

    // Calculating region size depending upon the number of processes
    if(number_of_processes == 1){
        region_size_row = M;
        region_size_col = N;
    }
    else if(number_of_processes == 2){
        region_size_row = M;
        region_size_col = N/2;
    }else if(number_of_processes == 4 || number_of_processes == 16){
        region_size_row = M/sqrt(number_of_processes);
        region_size_col = N/sqrt(number_of_processes);
    }else if(number_of_processes == 8){
        region_size_row = M/2;
        region_size_col = N/4;
    }else if(number_of_processes == 32){
        region_size_row = M/4;
        region_size_col = N/8;
    }

    float ** V = new float*[M];
    float * regions = new float[number_of_processes * region_size_row * region_size_col];
    float * worker_region = new float[region_size_col * region_size_row];
    float * col_right = new float[region_size_row];
    float * col_left = new float[region_size_row];
    float * row_below = new float[region_size_col];
    float * row_above = new float[region_size_col];

    bool convergence = false;
    int timestamp_index = 0;
    int left, right, below, above;

    if(taskid == MASTER){
        // Initializing the grid with default values
        for(int i = 0; i < M; ++i)
            V[i] = new float[N];
        for(int i = 0; i < M; i++){
            for(int j = 0; j < N; j++){
                V[i][j] = 0.0;
            }
        }
        for(int i = 0; i < M; i++){
                V[i][0] = 1.0;
                V[i][N-1] = 1.0;
        }
        for(int i = 0; i < N; i++){
                V[0][i] = 1.0;
                V[M-1][i] = 1.0;
        }
        
        int index = 0, col = 0, row = 0, mul = 0, col_mul = 0;
        int num_region = 0, dest = 0, flag = 0;

        // Creating regions from the grid V
        for (int i = 0; i < number_of_processes; i++){
            for (int j = 0; j < region_size_row; j++){
                col = region_size_col * col_mul;
                for (int k = 0; k < region_size_col; k++){
                    *(regions + i * region_size_row * region_size_col + j * region_size_col + k) = V[row][col];
                    col++;
                }
                row++;
            }
            if(col == N){
                mul = mul + 1;
                col = 0;
                col_mul = 0;
                if(flag == 0){
                    num_region = i;
                    flag = 1;
                }
            }
            else{
                col_mul = col_mul + 1;
                col = region_size_col * col_mul;
            }
            row = 0;
            row = region_size_row * mul;
        }

        // Determining the neighbours of a region and then sending the region and its neighbours to that that specifc worker task
        int org_region = num_region;
        for (int i = 1; i < number_of_processes; i++){
            dest = i;
            if (i<=num_region){
                left = i - 1;
                if(i != num_region){
                    right = i + 1;
                }else{
                    right = -1;
                }
            }else{
                num_region = num_region + org_region + 1;
                right = i + 1;
                left = -1;
            }
            if ((i - org_region - 1) >= 0){
                above = i - org_region - 1;
            }else{
                above = -1;
            }
            
            if ((i + org_region + 1) < number_of_processes){
                below = i + org_region + 1;
            }else{
                below = -1;
            }

            MPI_Send(regions + i * region_size_row * region_size_col, region_size_row * region_size_col, MPI_FLOAT, dest, BEGIN, MPI_COMM_WORLD);
            MPI_Send(&left, 1, MPI_INT, dest, BEGIN, MPI_COMM_WORLD);
            MPI_Send(&right, 1, MPI_INT, dest, BEGIN, MPI_COMM_WORLD);
            MPI_Send(&above, 1, MPI_INT, dest, BEGIN, MPI_COMM_WORLD);
            MPI_Send(&below, 1, MPI_INT, dest, BEGIN, MPI_COMM_WORLD);
        }

        // Determining the neighbours of the MASTER region depending upon the number of processes
        if (number_of_processes == 1)
            left = -1, right = -1, above = -1, below = -1;
        else if (number_of_processes == 2)
            left = -1, right = 1, above = -1, below = -1;
        else
            left = -1, right = 1, above = -1, below = org_region + 1;
        
        for (int i = 0; i < region_size_row; i++){
            for(int j = 0; j < region_size_col; j++){
                *((worker_region + i * region_size_col) + j) = *(regions + 0 * region_size_row * region_size_col + i * region_size_col + j);
            }
        }

        while(timestamp_index < timestamps){ 
            if (number_of_processes != 1){
                send(left, right, below, above, worker_region, col_left, col_right, row_below, row_above, region_size_row, region_size_col, status);
            }

            if(convergence == false){
                update(worker_region, col_left, col_right, row_below, row_above, left, right, above, below, timestamp_index, 
                taskid, region_size_row, region_size_col, convergence, C);
            }
            timestamp_index++;
       }
    }
    else if(taskid != MASTER){
        // Each worker thread receives its region and information about the neighbours of that region
        MPI_Recv(worker_region, region_size_row * region_size_col, MPI_FLOAT, MASTER, BEGIN, MPI_COMM_WORLD, &status);
        MPI_Recv(&left, 1, MPI_INT, MASTER, BEGIN, MPI_COMM_WORLD, &status);
        MPI_Recv(&right, 1, MPI_INT, MASTER, BEGIN, MPI_COMM_WORLD, &status);
        MPI_Recv(&above, 1, MPI_INT, MASTER, BEGIN, MPI_COMM_WORLD, &status);
        MPI_Recv(&below, 1, MPI_INT, MASTER, BEGIN, MPI_COMM_WORLD, &status);
    
        while(timestamp_index < timestamps){ 
            send(left, right, below, above, worker_region, col_left, col_right, row_below, row_above, region_size_row, region_size_col, status);
            if(convergence == false){
                update(worker_region, col_left, col_right, row_below, row_above, left, right, above, below, timestamp_index,
                    taskid, region_size_row, region_size_col, convergence, C);
            }
            timestamp_index++;
        }
    }
    MPI_Finalize();
}