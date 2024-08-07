#include <stdio.h>
#include <stdlib.h>

#include <iostream>

using namespace std;

typedef unsigned long ULONG;

// g++ example.cpp -o example
//./example

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

void calculator1()
{
    // For the sequential random number generator
    ULONG a = 1664525;
    ULONG c = 1013904223;
    ULONG m = 4294967296;
    ULONG sidelen = 65536;
    // sqrt of m
    ULONG numtrials = 10;

    ULONG i_prev = 12345; // Seed value
    for (ULONG n = 0; n < numtrials; ++n)
    {

        ULONG i_next = modlin(a, i_prev, c, m);
        i_prev = i_next;
        cout << "Random [" << n << "] value :" << i_next << endl;
        // Scale the random number to a random 2−d position
        ULONG ix = i_next % sidelen;
        ULONG iy = i_next / sidelen;
        // Scale current random integer to value from 0−1
        double x = rescale(sidelen, ix, -1, 1);
        double y = rescale(sidelen, iy, -1, 1);
        // Now we have an (x , y) pair generated from a single random integer
    }
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


ULONG leapFrog_A(ULONG x, int pow, ULONG mod_v)
{
    ULONG y = int_pow(x, pow);    
    return y % mod_v;
}


void calculator2(int pos, ULONG numtrials)
{
    // For the sequential random number generator
    ULONG a = 1664525;
    ULONG c = 1013904223;
    ULONG m = 4294967296;

    ULONG i_prev = 12345; // Seed value

    ULONG lfa = leapFrog_A(a, pos, m);
    ULONG lfc = leapfrog_C(c, a, pos, m);

    cout<<"a is "<< a<<", lfa is "<<lfa<<endl;
    cout<<"c is "<< c<<", lfc is "<<lfc<<endl;


    for (ULONG n = 0; n < numtrials; ++n)
    {
        cout << "Seed is " << i_prev << endl;
        ULONG i_next = modlin(lfa, i_prev, lfc, m);
        i_prev = i_next;
        cout << "numtrial [" << n << "] value :" << i_next << endl;
    }
}

void calculate_seeks(ULONG arr[], int size){
    ULONG a = 1664525;
    ULONG c = 1013904223;
    ULONG m = 4294967296;

    ULONG i_prev = 12345; // Seed value

    for (size_t i = 0; i < size; i++)
    {
        arr[i] = modlin(a, i_prev, c, m);
        i_prev = arr[i];
    }
    
}


int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        calculator1();
    }
    else
    {
        calculator2(2, 3);
    }
}