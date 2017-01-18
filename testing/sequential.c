#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include "stars.h"
#include "stars-spatial.h"

int main(int argc, char **argv)
// Example of how to use STARS library for spatial statistics.
// For more information on STARS structures look inside of header files.
{
    if(argc < 7)
    {
        printf("%d\n", argc);
        printf("spatial.out n block_size fixrank maxrank tol beta\n");
        exit(0);
    }
    int n = atoi(argv[1]), block_size = atoi(argv[2]);
    int fixrank = atoi(argv[3]), maxrank = atoi(argv[4]);
    double tol = atof(argv[5]), beta = atof(argv[6]);
    printf("\nn=%d, bs=%d, fr=%d, mr=%d, tol=%e, beta=%f\n",
            n, block_size, fixrank, maxrank, tol, beta);
    // Setting random seed
    srand(time(NULL));
    // Generate data for spatial statistics problem
    STARS_ssdata *data = STARS_gen_ssdata2(n, beta);
    int ndim = 2, shape[2] = {data->count, data->count}, info;
    char symm = 'S', dtype = 'd';
    // Init problem with given data and kernel
    STARS_Problem *P;
    info = STARS_Problem_new(&P, ndim, shape, symm, dtype, data, data,
            STARS_ssdata_block_exp_kernel, "Spatial Statistics example");
    STARS_Problem_info(P);
    // Init tiled cluster for tiled low-rank approximation
    STARS_Cluster *C;
    info = STARS_Cluster_new_tiled(&C, data, data->count, block_size);
    STARS_Cluster_info(C);
    // Init tiled division into admissible blocks
    STARS_BLRF *F;
    info = STARS_BLRF_new_tiled(&F, P, C, C, 'S');
    STARS_BLRF_info(F);
    // Approximate each admissible block
    STARS_BLRM *M;
    //info = STARS_BLRM_tiled_compress_algebraic_svd(&M, F, fixrank, tol, 1);
    info = starsh_blrm__dsdd(&M, F, tol, 1);
    // 0 for onfly=0
    // Print info about approximation
    STARS_BLRM_info(M);
    // Measure approximation error in Frobenius norm
    STARS_BLRM_error(M);
    printf("error, measured by starsh_blrm__dfe %e\n", starsh_blrm__dfe(M));
    Array *A;
    info = STARS_Problem_to_array(P, &A);
    Array *B;
    info = STARS_BLRM_to_matrix(M, &B);
    // Measure accuracy by dense matrices
    double diff, norm;
    info = Array_diff(A, B, &diff);
    info = Array_norm(A, &norm);
    printf("STARS_BLRM_to_matrix diff with Array: %e\n", diff/norm);
    // Check if this problem is good for Cholesky factorization
    //printf("Info of potrf: %d\n", Array_Cholesky(A, 'L'));
    // Free memory, consumed by array
    Array_free(A);
    int m = shape[0], k = 100;
    shape[1] = k;
    Array_new(&A, 2, shape, 'd', 'F');
    Array *resM, *resB;
    Array_new(&resM, 2, shape, 'd', 'F');
    Array_init_randn(A);
    Array_init_zeros(resM);
    starsh_blrm__dmml(M, k, A->data, m, resM->data, m);
    Array_dot(B, A, &resB);
    Array_diff(resM, resB, &diff);
    Array_norm(resB, &norm);
    printf("starsh_blrm__dmml check: %e\n", diff/norm);
    //Array_free(resM);
    //Array_free(resB);
    //Array_free(A);
    //Array_free(B);
    // Free memory, used by matrix in block low-rank format
    STARS_BLRM_free(M);
    // Free memory, used by block low-rank format
    STARS_BLRF_free(F);
    // Free memory, used by clusterization info
    STARS_Cluster_free(C);
    // Free memory, used by STARS_Problem instance
    STARS_Problem_free(P);
    return 0;
}
