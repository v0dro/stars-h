/*! @copyright (c) 2020 King Abdullah University of Science and
 *                      Technology (KAUST). All rights reserved.
 *
 * STARS-H is a software package, provided by King Abdullah
 *             University of Science and Technology (KAUST)
 *
 * Generate different functions for different dimensions. This hack improves
 * performance in certain cases. Value 'n' stands for general case, whereas all
 * other values correspond to static values of dimensionality.
 * will be replace by proposed values. If you want to use this file outside
 * STARS-H, simply do substitutions yourself.
 *
 * @file src/applications/mesh_deformation/virus.c
 * @version 0.1.1
 * @author Rabab Alomairy
 * @date 2020-05-09
 */

#include "common.h"
#include "starsh.h"
#include "starsh-rbf.h"
#include <inttypes.h>
#include <math.h>
#include <stdio.h>

/*! RBF Gaussian basis function
 * @param[in] x: Euclidean distance
 */
static double Gaussian(double x)
{
        return exp(-pow(x, 2));
}

/*! RBF Exponential basis function
 * @param[in] x: Euclidean distance
 */
static double Expon(double x)
{       
        return exp(-x);
}

/*! RBF Maternc1 basis function
 * @param[in] x: Euclidean distance
 */
static double Maternc1(double x)
{
        return exp(-x)+(1+x);
}

/*! RBF Maternc2 basis function
 * @param[in] x: Euclidean distance
 */
static double Maternc2(double x)
{
        return exp(-x)+(3+3*x+pow(x,2));
}

/*! RBF Quadratic basis function
 * @param[in] x: Euclidean distance
 */
static double QUAD(double x)
{
        return 1 + pow(x, 2);
}

/*! RBF Inverse Quadratic basis function
 * @param[in] x: Euclidean distance
 */
static double InvQUAD(double x)
{
	return 1 / (1 + pow(x, 2));
}

/*! RBF Inverse Multi-Quadratic basis function
 * @param[in] x: Euclidean distance
 */
static double InvMQUAD(double x)
{
        return 1 / sqrt(1 + pow(x, 2));
}

/*! RBF Thin plate spline basis function
 * @param[in] x: Euclidean distance
 */
static double TPS(double x)
{
        return pow(x, 2) * log(x);
}

/*! RBF Wendland basis function
 * @param[in] x: Euclidean distance
 */
static double Wendland(double x)
{
        if (x > 1)
                return 0;
        else
                return pow(1 - x, 4)*(4 * x + 1);
}

/*! RBF Continuous Thin plate spline basis function
 * @param[in] x: Euclidean distance
 */
static double CTPS(double x)
{
        if (x > 1)
                return 0;
        else
                return pow(1 - x, 5);
}

/*! Computing Euclidean distance
 * @param[in] x: Mesh Coordinates along x-axis
 * @param[in] y: Mesh Coordinates along y-axis
 */

static double diff(double*x, double*y)
{
        double r = 0;
        for (int i = 0; i < 3; i++)
                r = r + pow(x[i] - y[i], 2);
        return pow(r, 0.5);
}


/*! Fills matrix \f$ A \f$ with values
 * \f[
 *      A_{ij} = \frac{1}{r_{ij}},
 * \f]
 * \f$ r_{ij} \f$ is a distance between \f$i\f$-th and \f$j\f$-th mesh
 * points. No memory is allocated in this function!
 *
 * @param[in] nrows: Number of rows of \f$ A \f$.
 * @param[in] ncols: Number of columns of \f$ A \f$.
 * @param[in] irow: Array of row indexes.
 * @param[in] icol: Array of column indexes.
 * @param[in] row_data: Pointer to physical data (\ref STARSH_mddata object).
 * @param[in] col_data: Pointer to physical data (\ref STARSH_mddata object).
 * @param[out] result: Pointer to memory of \f$ A \f$.
 * @param[in] ld: Leading dimension of `result`.
 * */

void starsh_generate_3d_virus(int nrows, int ncols,
        STARSH_int *irow, STARSH_int *icol, void *row_data, void *col_data,
        void *result, int lda)
{
        int m, k;
        STARSH_mddata *data = row_data;
        double *mesh = data->particles.point;
        double rad = data->rad;

         if((data->numobj)>1 && (data->rad)<0) rad=0.25*(data->numobj)*sqrt(3); //Use the this formultation

         double *A= (double *)result;

        for(m=0;m<nrows;m++){
                int i0=irow[m];
                int posi=i0*3;
                double vi[3] = {mesh[posi], mesh[posi+1], mesh[posi+2]};
 
                for(k=0;k<ncols;k++){
                        int j0=icol[k];
                        int posj=j0*3;
                        double vj[3] = {mesh[posj], mesh[posj+1], mesh[posj+2]};
                        double d = diff(vi, vj) / (double)rad;
                        switch(data->kernel){
                           case 0: A[lda*k+m]=Gaussian(d);
                                   break;
                           case 1: A[lda*k+m]=Expon(d);
                                   break;
                           case 2: A[lda*k+m]=InvQUAD(d);
                                   break;
                           case 3: A[lda*k+m]=InvMQUAD(d);
                                   break;
                           case 4: A[lda*k+m]=Maternc1(d);
                                   break;
                           case 5: A[lda*k+m]=Maternc1(d);
                                   break;
                           case 6: A[lda*k+m]=TPS(d);
                                   break;
                           case 7: A[lda*k+m]=CTPS(d);
                                   break;
                           default: A[lda*k+m]=Wendland(d);
                                   break;
                        }
                        if( i0==j0 && (data->isreg)) A[lda*k+m]+=(data->reg);
                }
        }

}

/*! Fills matrix (RHS) \f$ A \f$ with values
 * @param[in] ld: Total number of mesh points.
 * @param[inout] ld: Pointer to memory of \f$ A \f$..    
*/
void starsh_generate_3d_virus_rhs(STARSH_int mesh_points, double *A)
{
    int i;

    for(i=0;i<mesh_points;i++)
       A[i]=0.01;
    for(i=mesh_points;i<2*mesh_points;i++)
       A[i]=-0.019;
    for(i=2*mesh_points;i<3*mesh_points;i++)
       A[i]=0.021;
}
