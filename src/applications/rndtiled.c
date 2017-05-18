/*! @copyright (c) 2017 King Abdullah University of Science and
 *                      Technology (KAUST). All rights reserved.
 *
 * @file rndtiled.c
 * @version 1.0.0.2
 * @author Aleksandr Mikhalev
 * @date 16 May 2017
 * */

#include "common.h"
#include "starsh.h"
#include "starsh-rndtiled.h"

void starsh_rndtiled_block_kernel(int nrows, int ncols, int *irow,
        int *icol, void *row_data, void *col_data, void *result)
{
//! Kernel for randomly generated tiled matrix.
//! @ingroup applications
    STARSH_rndtiled *data = row_data;
    int n = data->n;
    int nblocks = data->nblocks;
    int block_size = data->block_size;
    double add_diag = data->add_diag;
    double *U = data->U;
    double *S = data->S;
    double *buffer = result;
    for(int i = 0; i < nrows; i++)
    {
        int ii = irow[i];
        int ibrow = ii/block_size;
        for(int j = 0; j < ncols; j++)
        {
            int jj = icol[j];
            int jbcol = jj/block_size;
            double res = 0;
            for(int k = 0; k < block_size; k++)
                res += U[ii+k*n]*U[jj+k*n]*S[k];
            if(ii == jj)
                buffer[j*nrows+i] = res+add_diag;
            else
                buffer[j*nrows+i] = res;
        }
    }
}

int starsh_rndtiled_new(STARSH_rndtiled **data, int n, char dtype,
        int block_size, double decay, double add_diag)
//! Generate random tiled matrix by a special rule
/*! @ingroup applications
 * @param[out] data: Address to pointer to `STARSH_rndtiled` object.
 * @param[in] n: Size of matrix.
 * @param[in] dtype: precision ('s', 'd', 'c' or 'z').
 * @param[in] block_size: Size of tile in one dimension.
 * @param[in] decay: Decay of singular values.
 * @param[in] add_diag: Value to add to each diagonal element.
 * */
{
    if(dtype != 'd')
    {
        STARSH_ERROR("Only dtype='d' is supported");
        return 1;
    }
    if(n % block_size != 0)
    {
        STARSH_ERROR("N mod NB must be equal to 0");
        return 1;
    }
    int nblocks = n/block_size;
    int iseed[4] = {0, 0, 0, 1};
    double *U, *S, *tau, *work;
    int lwork = block_size;
    STARSH_MALLOC(U, n*block_size);
    STARSH_MALLOC(S, block_size);
    STARSH_MALLOC(tau, block_size);
    STARSH_MALLOC(work, lwork);
    LAPACKE_dlarnv_work(3, iseed, n*block_size, U);
    for(int i = 0; i < nblocks; i++)
    {
        LAPACKE_dgeqrf_work(LAPACK_COL_MAJOR, block_size, block_size,
                U+i*block_size, n, tau, work, lwork);
        LAPACKE_dorgqr_work(LAPACK_COL_MAJOR, block_size, block_size,
                block_size, U+i*block_size, n, tau, work, lwork);
    }
    free(work);
    free(tau);
    S[0] = 1.;
    for(int i = 1; i < block_size; i++)
        S[i] = S[i-1]*decay;
    STARSH_MALLOC(*data, 1);
    (*data)->n = n;
    (*data)->dtype = dtype;
    (*data)->nblocks = nblocks;
    (*data)->block_size = block_size;
    (*data)->U = U;
    (*data)->S = S;
    (*data)->add_diag = add_diag;
    return 0;
}

int starsh_rndtiled_new_va(STARSH_rndtiled **data, int n, char dtype,
        va_list args)
//! Generate random tiled blr-matrix with va_list.
//! For arguments look at starsh_rndtiled_new().
//! @ingroup applications
{
    int nb = n;
    double decay = 0;
    double add_diag = 0;
    int arg_type;
    if(dtype != 'd')
    {
        STARSH_ERROR("Only dtype='d' is supported");
        return 1;
    }
    while((arg_type = va_arg(args, int)) != 0)
    {
        switch(arg_type)
        {
            case STARSH_RNDTILED_NB:
                nb = va_arg(args, int);
                break;
            case STARSH_RNDTILED_DECAY:
                decay = va_arg(args, double);
                break;
            case STARSH_RNDTILED_DIAG:
                add_diag = va_arg(args, double);
                break;
            default:
                STARSH_ERROR("Wrong parameter type");
                return 1;
        }
    }
    return starsh_rndtiled_new(data, n, dtype, nb, decay, add_diag);
}

int starsh_rndtiled_new_el(STARSH_rndtiled **data, int n, char dtype, ...)
//! Generate random tiled blr-matrix with ellipsis.
//! For arguments look at starsh_rndtiled_new().
//! @ingroup applications
{
    va_list args;
    va_start(args, dtype);
    int info = starsh_rndtiled_new_va(data, n, dtype, args);
    va_end(args);
    return info;
}

int starsh_rndtiled_get_kernel(STARSH_kernel *kernel, STARSH_rndtiled *data,
        int type)
//! Select kernel (ignores type, since there is only one kernel).
//! @ingroup applications
{
    *kernel = starsh_rndtiled_block_kernel;
    return 0;
}

int starsh_rndtiled_free(STARSH_rndtiled *data)
//! Free data.
//! @ingroup applications
{
    free(data->U);
    free(data->S);
    free(data);
}
