// first.cpp Adding numbers using two nodes C++ version
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <iostream>

#include "mpi.h"

using namespace std;


/**
 * mpic++ a1.cpp -o a1test
 * mpirun -np 4 a1test 
 */

// always use argc and argv, as mpirun will pass the appropriate parms.

typedef unsigned long ULONG;

// For the sequential random number generator
const ULONG a = 1664525;
const ULONG c = 1013904223;
const ULONG m = 4294967296;
const ULONG sidelen = 65536;
// sqrt of m
// const ULONG numtrials = 1000000;
const ULONG numtrials = 4;

// Evaluate ( ax+c ) mod m
ULONG modlin(ULONG a, ULONG x, ULONG c, ULONG m)
{
  return (a * x + c) % m;
}

// Put integer n in range x1 , x2 with the maximum integer value
double rescale(ULONG N, ULONG n, double x1, double x2)
{
  double f = static_cast<double>(n) / static_cast<double>(N);
  return x1 + f * (x2 - x1);
}


ULONG int_pow(ULONG x, int pow)
{
    ULONG y = 1;
    for (size_t i = 0; i < pow; ++i)
    {
        y = (y * x);
    }
    return y;
}

ULONG leapfrog_C(ULONG c, ULONG a, int pow, ULONG mode_v)
{
    ULONG total_a = 0;
    for (size_t i = 0; i < pow; i++)
    {
        total_a += int_pow(a, i);
    }
    return (c * total_a) % mode_v;
}


ULONG leapfrog_A(ULONG x, int pow, ULONG mod_v)
{
    ULONG y = int_pow(x, pow);    
    return y % mod_v;
}

ULONG calculate(ULONG i_prev, int interval, int pid)
{
  ULONG total_count = 0;

  ULONG lfa = leapfrog_A(a, interval, m);
  ULONG lfc = leapfrog_C(c, a, interval, m);

  for (ULONG n = 0; n < numtrials; ++n)
  {
    cout << "MyID[" << pid << "]" << ",start Random Number is: " << i_prev << std::endl;

    ULONG i_next = modlin(lfa, i_prev, lfc, m);
    i_prev = i_next;
    // Scale the random number to a random 2−d position
    ULONG ix = i_next % sidelen;
    ULONG iy = i_next / sidelen;
    // Scale current random integer to value from 0−1
    double x = rescale(sidelen, ix, -1, 1);
    double y = rescale(sidelen, iy, -1, 1);
    // Now we have an (x , y) pair generated from a single random integer
    double r = sqrt(x * x + y * y);
    total_count = (r >= 1) ? (++total_count) : total_count;
    std::cout << "MyID[" << pid << "]" << ",Random Number is: " << i_next << " ,ix: " << ix << " , iy: " << iy << " ,x: " << x << " ,y:" << y << " , R: " << r << std::endl;
  }
  return total_count;
}

void calculate_seeks(ULONG seek, ULONG arr[], int p)
{
  ULONG i_prev = seek;
  for (size_t i = 0; i < p; i++)
  {
    arr[i] = modlin(a, i_prev, c, m);
    i_prev = arr[i];
  }
}

int main(int argc, char *argv[])
{
  MPI::Init(argc, argv);

  // What is my ID and how many processes are in this pool?
  int myid = MPI::COMM_WORLD.Get_rank();
  int numproc = MPI::COMM_WORLD.Get_size();

  std::cout << "This is id " << myid << " out of " << numproc << std::endl;

  if (myid == 0)
  {

    ULONG seek = 12345; // Seed value
    ULONG irecv = 0;
    ULONG total_count = 0;

    //calculate the seeks
    ULONG arr[numproc];
    calculate_seeks(seek, arr, numproc);
    cout<<"Seek array is:["<<arr[0]<<","<<arr[1]<<","<<arr[2]<<","<<arr[3]<<"]"<<endl;

    // Send message to each slave process and wait for response
    for (int n = 1; n < numproc; ++n)
    {
      double time0 = MPI::Wtime();
      
      // Master sends seed to slave    Will it block the for loop?????
      MPI::COMM_WORLD.Send(&arr[n], 1, MPI::UNSIGNED_LONG, n, 0);

      // receive sends result from slave
      MPI::COMM_WORLD.Recv(&irecv, 1, MPI::UNSIGNED_LONG, n, 0);


      // Elapsed time in milliseconds
      double telapse = 1000 * (MPI::Wtime() - time0);
      // std::cout << "myid[" << n << "] processes time:" << telapse << " . number:"<< irecv<<std::endl;
    }



    //calculate the number of value smaller than 1
    total_count = calculate(arr[myid], numproc, myid);


    //try to receive the result from 
    for (int n = 1; n < numproc; ++n){
      ULONG iresult = 0;
      MPI::COMM_WORLD.Recv(&iresult, 1, MPI::UNSIGNED_LONG, n, 0);
      //sum up the result
      total_count += iresult;
      MPI::COMM_WORLD.Send(&total_count, 1, MPI::UNSIGNED_LONG, n, 0);
    }

    std::cout << "The final result is " << (total_count/(numproc*numtrials))*4 << std::endl;
  }

  else
  {

    // Slave waits to receive 'N' from master
    ULONG i_prev;
    std::cout << "The id "<<myid<<" waiting"<<std::endl;
    MPI::COMM_WORLD.Recv(&i_prev, 1, MPI::UNSIGNED_LONG, 0, 0);
    MPI::COMM_WORLD.Send(&i_prev, 1, MPI::UNSIGNED_LONG, 0, 0);

    ULONG sum1 = 0;

    sum1 = calculate(i_prev, numproc, myid);

    // Slave sends 'sum1' to master
    MPI::COMM_WORLD.Send(&sum1, 1, MPI::UNSIGNED_LONG, 0, 0);
    MPI::COMM_WORLD.Recv(&sum1, 1, MPI::UNSIGNED_LONG, 0, 0);
    std::cout << "The id ["<<myid<<"] calculation finish"<<std::endl;
  }
  MPI::Finalize();
}
