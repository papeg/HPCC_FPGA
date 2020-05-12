//
// Created by Marius Meyer on 04.12.19.
//

/*
Copyright (c) 2019 Marius Meyer

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "linpack_functionality.hpp"

/* C++ standard library headers */
#include <memory>
#include <random>

/* Project's headers */
#include "execution.h"
#include "cxxopts.hpp"
#include "setup/fpga_setup.hpp"
#include "setup/common_benchmark_io.hpp"
#include "parameters.h"


/**
Prints the execution results to stdout

@param results The execution results
*/
void
printResults(std::shared_ptr<bm_execution::ExecutionTimings> results, unsigned matrix_size) {

    std::cout << std::setw(ENTRY_SPACE)
              << "best" << std::setw(ENTRY_SPACE) << "mean"
              << std::setw(ENTRY_SPACE) << "GFLOPS" << std::endl;

    // Calculate performance for kernel execution plus data transfer
    double tmean = 0;
    double tmin = std::numeric_limits<double>::max();

    // GFLOPs for calculation of both GEFA and GESL.
    // Currently only GEFA is calculated on the FPGA so GFLOPS have to be
    // reduced.
    // double gflops = ((2.0e0*(dataSize*dataSize*dataSize))/3.0
    //                 + 2.0*(dataSize*dataSize)) / 1.0e9;
    // TODO: Change this when GESL is also calculated on FPGA
    double gflops = (2.0e0*(static_cast<double>(matrix_size)*static_cast<double>(matrix_size)*static_cast<double>(matrix_size)))/3.0/1.0e9;
    for (double currentTime : results->timings) {
        tmean +=  currentTime;
        if (currentTime < tmin) {
            tmin = currentTime;
        }
    }
    tmean = tmean / results->timings.size();

    std::cout << std::setw(ENTRY_SPACE)
              << tmin << std::setw(ENTRY_SPACE) << tmean
              << std::setw(ENTRY_SPACE) << (gflops / tmin)
              << std::endl;

}


void generateInputData(HOST_DATA_TYPE* A, HOST_DATA_TYPE* b, cl_int* ipvt, unsigned matrix_size, HOST_DATA_TYPE* norma) {
    std::mt19937 gen(7);
    std::uniform_real_distribution<> dis(-1.0, 1.0);
    *norma = 0.0;
    for (int j = 0; j < matrix_size; j++) {
        for (int i = 0; i < matrix_size; i++) {
            A[matrix_size*i+j] = dis(gen);
            *norma = (A[matrix_size*i+j] > *norma) ? A[matrix_size*i+j] : *norma;
        }
    }
    for (int i = 0; i < matrix_size; i++) {
        b[i] = 0.0;
    }
    for (int j = 0; j < matrix_size; j++) {
        for (int i = 0; i < matrix_size; i++) {
            b[j] += A[matrix_size*j+i];
        }
    }

}

double
checkLINPACKresults(const HOST_DATA_TYPE* b_res, unsigned n) {
    auto a = new HOST_DATA_TYPE[n*n];
    HOST_DATA_TYPE norma;
    auto x = new HOST_DATA_TYPE[n];
    auto b = new HOST_DATA_TYPE[n];
    /*     compute a residual to verify results.  */

    for (int i = 0; i < n; i++) {
        x[i] = b_res[i];
    }

    auto ipvt = new cl_int[n];
    generateInputData(a,b,ipvt,n, &norma);
    for (int i = 0; i < n; i++) {
        b[i] = -b[i];
    }
    dmxpy(n, b, n, n, x, a);
    HOST_DATA_TYPE resid = 0.0;
    HOST_DATA_TYPE normx = 0.0;

    for (int i = 0; i < n; i++) {
        resid = (resid > fabs(b[i])) ? resid : fabs(b[i]);
        normx = (normx > fabs(x[i])) ? normx : fabs(x[i]);
    }

    HOST_DATA_TYPE eps = std::numeric_limits<HOST_DATA_TYPE>::epsilon();
    HOST_DATA_TYPE residn = resid / (n*norma*normx*eps);
    //std::cout << resid << ", " << norma << ", " << normx << std::endl;
    std::cout << "  norm. resid        resid       "\
                 "machep       x[0]-1     x[n-1]-1" << std::endl;
    std::cout << std::setw(ENTRY_SPACE) << residn << std::setw(ENTRY_SPACE)
              << resid << std::setw(ENTRY_SPACE) << eps
              << std::setw(ENTRY_SPACE) << x[0]-1 << std::setw(ENTRY_SPACE)
              << x[n-1]-1 << std::endl;

    delete [] a;
    delete [] x;
    delete [] b;
    delete [] ipvt;
    return residn;
}

/**
Standard LU factorization on a block with fixed size

Case 1 of Zhangs description
*/
void
gefa_ref(HOST_DATA_TYPE* a, unsigned n, unsigned lda, cl_int* ipvt) {
    for (int i = 0; i < n; i++) {
        ipvt[i] = i;
    }
    // For each diagnonal element
    for (int k = 0; k < n - 1; k++) {
        HOST_DATA_TYPE max_val = fabs(a[k * lda + k]);
        int pvt_index = k;
        for (int i = k + 1; i < n; i++) {
            if (max_val < fabs(a[i * lda + k])) {
                pvt_index = i;
                max_val = fabs(a[i * lda + k]);
            }
        }

        for (int i = k; i < n; i++) {
            HOST_DATA_TYPE tmp_val = a[k * lda + i];
            a[k * lda + i] = a[pvt_index * lda + i];
            a[pvt_index * lda + i] = tmp_val;
        }
        ipvt[k] = pvt_index;

        // For each element below it
        for (int i = k + 1; i < n; i++) {
            a[i * lda + k] *= -1.0 / a[k * lda + k];
        }
        // For each column right of current diagonal element
        for (int j = k + 1; j < n; j++) {
            // For each element below it
            for (int i = k+1; i < n; i++) {
                a[i * lda + j] += a[i * lda + k] * a[k * lda + j];
            }
        }

#ifdef DEBUG
        std::cout << "A(k=" << k <<"): " << std::endl;
                for (int i= 0; i < n; i++) {
                    for (int j=0; j < n; j++) {
                        std::cout << a[i*lda + j] << ", ";
                    }
                    std::cout << std::endl;
                }
                std::cout <<  std::endl;
#endif

    }
}

void
gesl_ref(HOST_DATA_TYPE* a, HOST_DATA_TYPE* b, cl_int* ipvt, unsigned n, unsigned lda) {
    auto b_tmp = new HOST_DATA_TYPE[n];
#pragma omp parallel default(shared)
    {
#pragma omp for
        for (int k = 0; k < n; k++) {
            b_tmp[k] = b[k];
        }

        // solve l*y = b
        // For each row in matrix
#pragma omp single
        for (int k = 0; k < n - 1; k++) {
            if (ipvt[k] != k) {
                HOST_DATA_TYPE tmp = b_tmp[k];
                b_tmp[k] = b_tmp[ipvt[k]];
                b_tmp[ipvt[k]] = tmp;
            }
            // For each row below add
#pragma omp parallel for
            for (int i = k + 1; i < n; i++) {
                // add solved upper row to current row
                b_tmp[i] += b_tmp[k] * a[lda * i + k];
            }
        }

        // now solve  u*x = y
#pragma omp single
        for (int k = n - 1; k >= 0; k--) {
            b_tmp[k] = b_tmp[k] / a[lda * k + k];
#pragma omp parallel for
            for (int i = 0; i < k; i++) {
                b_tmp[i] -= b_tmp[k] * a[lda * i + k];
            }
        }
#pragma omp for
        for (int k = 0; k < n; k++) {
            b[k] = b_tmp[k];
        }
    }
    delete [] b_tmp;
}

void dmxpy(unsigned n1, HOST_DATA_TYPE* y, unsigned n2, unsigned ldm, HOST_DATA_TYPE* x, HOST_DATA_TYPE* m) {
    for (int i=0; i < n1; i++) {
        for (int j=0; j < n2; j++) {
            y[i] = y[i] + x[j] * m[ldm*i + j];
        }
    }
}

