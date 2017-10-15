/*! @copyright (c) 2017 King Abdullah University of Science and
 *                      Technology (KAUST). All rights reserved.
 *
 * STARS-H is a software package, provided by King Abdullah
 *             University of Science and Technology (KAUST)
 *
 * @file testing/mpi_spatial.c
 * @version 1.0.0
 * @author Aleksandr Mikhalev
 * @date 2017-08-22
 * */

#ifdef MKL
    #include <mkl.h>
#else
    #include <cblas.h>
    #include <lapacke.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>
#include "starsh.h"
#include "starsh-spatial.h"

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    int mpi_size, mpi_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    if(argc != 10)
    {
        if(mpi_rank == 0)
        {
            printf("%d arguments provided, but 11 are needed\n",
                    argc-1);
            printf("mpi_spatial ndim placement kernel beta nu N block_size "
                    "maxrank tol\n");
        }
        MPI_Finalize();
        exit(1);
    }
    int problem_ndim = atoi(argv[1]);
    int place = atoi(argv[2]);
    int kernel_type = atoi(argv[3]);
    double beta = atof(argv[4]);
    double nu = atof(argv[5]);
    int N = atoi(argv[6]);
    int block_size = atoi(argv[7]);
    int maxrank = atoi(argv[8]);
    double tol = atof(argv[9]);
    double noise = 0;
    int onfly = 0;
    char symm = 'N', dtype = 'd';
    int ndim = 2;
    STARSH_int shape[2] = {N, N};
    // Possible values can be found in documentation for enum
    // STARSH_PARTICLES_PLACEMENT
    int nrhs = 1;
    int info;
    srand(0);
    // Init STARS-H
    starsh_init();
    // Generate data for spatial statistics problem
    STARSH_ssdata *data;
    STARSH_kernel *kernel;
    //starsh_gen_ssdata(&data, &kernel, n, beta);
    info = starsh_application((void **)&data, &kernel, N, dtype,
            STARSH_SPATIAL, kernel_type, STARSH_SPATIAL_NDIM, problem_ndim,
            STARSH_SPATIAL_BETA, beta, STARSH_SPATIAL_NU, nu,
            STARSH_SPATIAL_NOISE, noise, STARSH_SPATIAL_PLACE, place, 0);
    //starsh_particles_write_to_file_pointer_ascii(&data->particles, stdout);
    if(info != 0)
    {
        if(mpi_rank == 0)
            printf("Problem was NOT generated (wrong parameters)\n");
        MPI_Finalize();
        exit(1);
    }
    // Init problem with given data and kernel and print short info
    STARSH_problem *P;
    starsh_problem_new(&P, ndim, shape, symm, dtype, data, data,
            kernel, "Spatial Statistics example");
    if(mpi_rank == 0)
        starsh_problem_info(P); 
    // Init plain clusterization and print info
    STARSH_cluster *C;
    starsh_cluster_new_plain(&C, data, N, block_size);
    if(mpi_rank == 0)
        starsh_cluster_info(C);
    // Init tlr division into admissible blocks and print short info
    STARSH_blrf *F;
    STARSH_blrm *M;
    starsh_blrf_new_tlr_mpi(&F, P, symm, C, C);
    if(mpi_rank == 0)
        starsh_blrf_info(F);
    // Approximate each admissible block
    MPI_Barrier(MPI_COMM_WORLD);
    double time1 = MPI_Wtime();
    info = starsh_blrm_approximate(&M, F, maxrank, tol, onfly);
    if(info != 0)
    {
        if(mpi_rank == 0)
            printf("Approximation was NOT computed due to error\n");
        MPI_Finalize();
        exit(1);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    time1 = MPI_Wtime()-time1;
    if(mpi_rank == 0)
    {
        starsh_blrf_info(F);
        starsh_blrm_info(M);
    }
    if(mpi_rank == 0)
        printf("TIME TO APPROXIMATE: %e secs\n", time1);
    // Measure approximation error
    MPI_Barrier(MPI_COMM_WORLD);
    time1 = MPI_Wtime();
    double rel_err = starsh_blrm__dfe_mpi(M);
    MPI_Barrier(MPI_COMM_WORLD);
    time1 = MPI_Wtime()-time1;
    if(mpi_rank == 0)
    {
        printf("TIME TO MEASURE ERROR: %e secs\nRELATIVE ERROR: %e\n",
                time1, rel_err);
        if(rel_err/tol > 10.)
        {
            printf("Resulting relative error is too big\n");
            MPI_Finalize();
            exit(1);
        }
    }
    MPI_Finalize();
    return 0;
}
