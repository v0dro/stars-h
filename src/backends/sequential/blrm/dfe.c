/*! @copyright (c) 2017 King Abdullah University of Science and
 *                      Technology (KAUST). All rights reserved.
 *
 * @file backends/sequential/blrm/dfe.c
 * @version 1.0.0.2
 * @author Aleksandr Mikhalev
 * @date 16 May 2017
 * */

#include "common.h"
#include "starsh.h"

double starsh_blrm__dfe(STARSH_blrm *M)
//! Approximation error in Frobenius norm of double precision matrix.
//! @ingroup blrm
{
    STARSH_blrf *F = M->format;
    STARSH_problem *P = F->problem;
    STARSH_kernel kernel = P->kernel;
    // Shortcuts to information about clusters
    STARSH_cluster *R = F->row_cluster, *C = F->col_cluster;
    void *RD = R->data, *CD = C->data;
    // Number of far-field and near-field blocks
    size_t nblocks_far = F->nblocks_far, nblocks_near = F->nblocks_near, bi;
    size_t nblocks = nblocks_far+nblocks_near;
    // Shortcut to all U and V factors
    Array **U = M->far_U, **V = M->far_V;
    // Special constant for symmetric case
    double sqrt2 = sqrt(2.);
    // Temporary arrays to compute norms more precisely with dnrm2
    double block_norm[nblocks], far_block_diff[nblocks_far];
    double *far_block_norm = block_norm;
    double *near_block_norm = block_norm+nblocks_far;
    char symm = F->symm;
    // Simple cycle over all far-field blocks
    for(bi = 0; bi < nblocks_far; bi++)
    {
        // Get indexes and sizes of block row and column
        int i = F->block_far[2*bi];
        int j = F->block_far[2*bi+1];
        int nrows = R->size[i];
        int ncols = C->size[j];
        // Rank of a block
        int rank = M->far_rank[bi];
        // Temporary array for more precise dnrm2
        double *D, D_norm[ncols];
        size_t D_size = (size_t)nrows*(size_t)ncols;
        STARSH_MALLOC(D, D_size);
        // Get actual elements of a block
        kernel(nrows, ncols, R->pivot+R->start[i], C->pivot+C->start[j],
                RD, CD, D);
        // Get Frobenius norm of a block
        for(size_t k = 0; k < ncols; k++)
            D_norm[k] = cblas_dnrm2(nrows, D+k*nrows, 1);
        double tmpnorm = cblas_dnrm2(ncols, D_norm, 1);
        far_block_norm[bi] = tmpnorm;
        // Get difference of initial and approximated block
        cblas_dgemm(CblasColMajor, CblasNoTrans, CblasTrans, nrows, ncols,
                rank, -1., U[bi]->data, nrows, V[bi]->data, ncols, 1.,
                D, nrows);
        // Compute Frobenius norm of the latter
        for(size_t k = 0; k < ncols; k++)
            D_norm[k] = cblas_dnrm2(nrows, D+k*nrows, 1);
        free(D);
        double tmpdiff = cblas_dnrm2(ncols, D_norm, 1);
        far_block_diff[bi] = tmpdiff;
        if(i != j && symm == 'S')
        {
            // Multiply by square root of 2 in symmetric case
            // (work on 1 block instead of 2 blocks)
            far_block_norm[bi] *= sqrt2;
            far_block_diff[bi] *= sqrt2;
        }
    }
    if(M->onfly == 0)
        // Simple cycle over all near-field blocks
        for(bi = 0; bi < nblocks_near; bi++)
        {
            // Get indexes and sizes of corresponding block row and column
            int i = F->block_near[2*bi];
            int j = F->block_near[2*bi+1];
            int nrows = R->size[i];
            int ncols = C->size[j];
            // Compute norm of a block
            double *D = M->near_D[bi]->data, D_norm[ncols];
            for(size_t k = 0; k < ncols; k++)
                D_norm[k] = cblas_dnrm2(nrows, D+k*nrows, 1);
            near_block_norm[bi] = cblas_dnrm2(ncols, D_norm, 1);
            if(i != j && symm == 'S')
                // Multiply by square root of 2 in symmetric case
                near_block_norm[bi] *= sqrt2;
        }
    else
        // Simple cycle over all near-field blocks
        for(bi = 0; bi < nblocks_near; bi++)
        {
            // Get indexes and sizes of corresponding block row and column
            int i = F->block_near[2*bi];
            int j = F->block_near[2*bi+1];
            int nrows = R->size[i];
            int ncols = C->size[j];
            double *D, D_norm[ncols];
            // Allocate temporary array and fill it with elements of a block
            STARSH_MALLOC(D, (size_t)nrows*(size_t)ncols);
            kernel(nrows, ncols, R->pivot+R->start[i], C->pivot+C->start[j],
                    RD, CD, D);
            // Compute norm of a block
            for(size_t k = 0; k < ncols; k++)
                D_norm[k] = cblas_dnrm2(nrows, D+k*nrows, 1);
            // Free temporary buffer
            free(D);
            near_block_norm[bi] = cblas_dnrm2(ncols, D_norm, 1);
            if(i != j && symm == 'S')
                // Multiply by square root of 2 ub symmetric case
                near_block_norm[bi] *= sqrt2;
        }
    // Get difference of initial and approximated matrices
    double diff = cblas_dnrm2(nblocks_far, far_block_diff, 1);
    // Get norm of initial matrix
    double norm = cblas_dnrm2(nblocks, block_norm, 1);
    return diff/norm;
}
