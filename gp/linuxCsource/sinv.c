/* SINV    Evaluate the sparse inverse matrix
 *
 * z = sinv(A)  returns the elements of inv(A)_ij, for which A_ij
 *      is different from zero. 
 *
 *    See Vanhatalo and Vehtari (2008) for details. 
 *
 *
 *   Note! The function works only for symmetric matrices!
 *
 */

/* -----------------------------------------------------------------------------
 * Copyright (C) 2005-2006 Timothy A. Davis
 * Copyright (c) 2008      Jarno Vanhatalo
 *
 * This software is distributed under the GNU General Public
 * License (version 2 or later); please refer to the file
 * License.txt, included with the software, for details.
 /* -----------------------------------------------------------------------------

 /* -----------------------------------------------------------------------------
 * The function uses CHOLMOD/MATLAB Module by Timothy A. Davis. Parts of the
 * code are copied from ldlchol.c function.
 *
 * The CHOLMOD/MATLAB Module is licensed under Version 2.0 of the GNU
 * General Public License. CHOLMOD is also available under other licenses; contact
 * authors for details. http://www.cise.ufl.edu/research/sparse
 * MATLAB(tm) is a Trademark of The MathWorks, Inc.
 * -------------------------------------------------------------------------- */

#include <stdio.h>
#include "cholmod_matlab.h"
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

void cumsum2 (mwIndex *p, mwIndex *c, int n);

void mexFunction
(
    int	nargout,
    mxArray *pargout [ ],
    int	nargin,
    const mxArray *pargin [ ]
)
{
  double dummy = 0, beta [2], *px, *C, *Ct, *C2, *fil, *Zt, *zt, done=1.0, *zz, dzero=0.0;
  cholmod_sparse Amatrix, *A, *Lsparse ;
  cholmod_factor *L ;
  cholmod_common Common, *cm ;
  Int n, minor ;
  mwIndex *I, *J, *Jt, *It, *I2, *J2;
  mwSize nnz, nnzlow;
  int i, j, k, l, k2, ik, h, *w, *w2, m, nz = 0, lfi, *r, *q;
  const int one=1;
  mxArray *Am, *Bm;
  char *uplo="L", *trans="N";
    

    /* ---------------------------------------------------------------------- */
    /* start CHOLMOD and set parameters */ 
    /* ---------------------------------------------------------------------- */

    cm = &Common ;
    cholmod_l_start (cm) ;
    sputil_config (SPUMONI, cm) ;

    /* convert to packed LDL' when done */
    cm->final_asis = FALSE ;
    cm->final_super = FALSE ;
    cm->final_ll = FALSE ;
    cm->final_pack = TRUE ;
    cm->final_monotonic = TRUE ;

    /* since numerically zero entries are NOT dropped from the symbolic
     * pattern, we DO need to drop entries that result from supernodal
     * amalgamation. */
    cm->final_resymbol = TRUE ;

    cm->quick_return_if_not_posdef = (nargout < 2) ;

    /* This will disable the supernodal LL', which will be slow. */
    /* cm->supernodal = CHOLMOD_SIMPLICIAL ; */

    /* ---------------------------------------------------------------------- */
    /* get inputs */
    /* ---------------------------------------------------------------------- */

    if (nargin > 3)
    {
	mexErrMsgTxt ("usage: Z = sinv(A)") ;
    }

    n = mxGetM (pargin [0]) ;
    m = mxGetM (pargin [0]) ;

    if (!mxIsSparse (pargin [0]))
    {
    	mexErrMsgTxt ("A must be sparse") ;
    }
    if (n != mxGetN (pargin [0]))
    {
    	mexErrMsgTxt ("A must be square") ;
    }

    /* get sparse matrix A, use tril(A)  */
    A = sputil_get_sparse (pargin [0], &Amatrix, &dummy, -1) ; 

    A->stype = -1 ;	    /* use lower part of A */
    beta [0] = 0 ;
    beta [1] = 0 ;

    /* ---------------------------------------------------------------------- */
    /* analyze and factorize */
    /* ---------------------------------------------------------------------- */

    L = cholmod_l_analyze (A, cm) ;
    cholmod_l_factorize_p (A, beta, NULL, 0, L, cm) ;

    if (cm->status != CHOLMOD_OK)
    {
	mexErrMsgTxt ("matrix is not positive definite") ;
    }

    /* ---------------------------------------------------------------------- */
    /* convert L to a sparse matrix */
    /* ---------------------------------------------------------------------- */

    Lsparse = cholmod_l_factor_to_sparse (L, cm) ;
    if (Lsparse->xtype == CHOLMOD_COMPLEX)
    {
      mexErrMsgTxt ("matrix is complex") ;
    }

    /* ---------------------------------------------------------------------- */
    /* Set the sparse Cholesky factorization in Matlab format */
    /* ---------------------------------------------------------------------- */
    Am = sputil_put_sparse (&Lsparse, cm) ;
    I = mxGetIr(Am);
    J = mxGetJc(Am);
    C = mxGetPr(Am);
    nnz = mxGetNzmax(Am);
          
    /* Evaluate the sparse inverse */
    C[nnz-1] = 1/C[J[m-1]];               /* set the last element of sparse inverse */
    fil = mxCalloc(1,sizeof(double));
    zt = mxCalloc(1,sizeof(double));
    Zt = mxCalloc(1,sizeof(double));
    zz = mxCalloc(1,sizeof(double));
    for (j=m-2;j>=0;j--){
      lfi = J[j+1]-(J[j]+1);
      fil = mxRealloc(fil,(size_t)(lfi*sizeof(double)));
      for (i=0;i<lfi;i++) fil[i] = C[J[j]+i+1];                /* take the j'th lower triangular column of the Cholesky */
      zt = mxRealloc(zt,(size_t)(lfi*sizeof(double)));         /* memory for the sparse inverse elements to be evaluated */
      Zt = mxRealloc(Zt,(size_t)(lfi*lfi*sizeof(double)));     /* memory for the needed sparse inverse elements */
      
      /* Set the lower triangular for Zt */
      k2 = 0;
      for (k=J[j]+1;k<J[j+1];k++){
	ik = I[k];
	h = k2;
	for (l=J[ik];l<=J[ik+1];l++){
	  if (I[l] == I[ J[j]+h+1 ]){
	    Zt[h+lfi*k2] = C[l];
	    h++;
	  }
	}
	k2++;
      }

      if (lfi > 0)
	{
	   /* evaluate zt = fil*Zt */
	  dsymv_(uplo, &lfi, &done, Zt, &lfi, fil, &one, &dzero, zt, &one);
	  
	  /* Set the evaluated sparse inverse elements, zt, into C */
	  k=lfi-1;
	  for (i = J[j+1]-1; i>J[j] ; i--){
	    C[i] = -zt[k];	
	    k--;
	  }
	  /* evaluate the j'th diagonal of sparse inverse */
	  dgemv_(trans, &one, &lfi, &done, fil, &one, zt, &one, &dzero, zz, &one); 
	  C[J[j]] = 1/C[J[j]] + zz[0];
	  	}
      else
	{
	  /* evaluate the j'th diagonal of sparse inverse */
	  C[J[j]] = 1/C[J[j]];

	} 

    }
    
    /* Free the temporary variables */
    mxFree(fil);
    mxFree(zt);
    mxFree(Zt);
    mxFree(zz); 

    /* ---------------------------------------------------------------------- */
    /* Permute the elements according to r(q) = 1:n */
    /* ---------------------------------------------------------------------- */
    
    Bm = mxCreateSparse(m, m, nnz, mxREAL) ;     
    It = mxGetIr(Bm);
    Jt = mxGetJc(Bm);
    Ct = mxGetPr(Bm);                            /* Ct = C(r,r)*/ 

    r = L->Perm;                                 /* fill reducing ordering */
    w = mxCalloc(m,sizeof(int));                 /* column counts of Am */
       
    /* count entries in each column of Bm */
    for (j=0; j<m; j++){
      k = r ? r[j] : j ;       /* column j of Bm is column k of Am */
      for (l=J[j] ; l<J[j+1] ; l++){
	i = I[l];
	ik = r ? r[i] : i ;    /* row i of Bm is row ik of Am */
	w[ max(ik,k) ]++;
      }
    }
    cumsum2(Jt, w, m);
    for (j=0; j<m; j++){
      k = r ? r[j] : j ;             /* column j of Bm is column k of Am */
      for (l=J[j] ; l<J[j+1] ; l++){
	i= I[l];
	ik = r ? r[i] : i ;          /* row i of Bm is row ik of Am */
	It [k2 = w[max(ik,k)]++ ] = min(ik,k);
	Ct[k2] = C[l];
      }
    }
    mxFree(w);
    
    /* ---------------------------------------------------------------------- */
    /* Transpose the permuted (upper triangular) matrix Bm into Am */
    /* (this way we get sorted columns)                            */
    /* ---------------------------------------------------------------------- */
    w = mxCalloc(m,sizeof(int));                 
    for (i=0 ; i<Jt[m] ; i++) w[It[i]]++;        /* row counts of Bm */
    cumsum2(J, w, m);                            /* row pointers */
    for (j=0 ; j<m ; j++){
      for (i=Jt[j] ; i<Jt[j+1] ; i++){
	I[ l=w[ It[i] ]++ ] = j;
	C[l] = Ct[i];
      }
    }
    mxFree(w);
    mxDestroyArray(Bm);
    

    /* ---------------------------------------------------------------------- */
    /* Fill the upper triangle of the sparse inverse */
    /* ---------------------------------------------------------------------- */

    w = mxCalloc(m,sizeof(int));        /* workspace */
    w2 = mxCalloc(m,sizeof(int));       /* workspace */
    for (k=0;k<J[m];k++) w[I[k]]++;     /* row counts of the lower triangular */
    for (k=0;k<m;k++) w2[k] = w[k] + J[k+1] - J[k] - 1;   /* column counts of the sparse inverse */
    
    nnz = (mwSize)(2*nnz - m);                    /* The number of nonzeros in Z */
    pargout[0] = mxCreateSparse(m,m,(mwSize)nnz,mxREAL);   /* The sparse matrix */
    It = mxGetIr(pargout[0]);
    Jt = mxGetJc(pargout[0]);
    Ct = mxGetPr(pargout[0]);

    cumsum2(Jt, w2, m);               /* column starting points */
    for (j = 0 ; j < m ; j++){           /* fill the upper triangular */
      for (k = J[j] ; k < J[j+1] ; k++){
	It[l = w2[ I[k]]++] = j ;	 /* place C(i,j) as entry Ct(j,i) */
	if (Ct) Ct[l] = C[k] ;
      }
    }
    for (j = 0 ; j < m ; j++){           /* fill the lower triangular */
      for (k = J[j]+1 ; k < J[j+1] ; k++){
	It[l = w2[j]++] = I[k] ;         /* place C(j,i) as entry Ct(j,i) */
	if (Ct) Ct[l] = C[k] ;
      }
    }
    
    mxFree(w2);
    mxFree(w);    
    mxDestroyArray(Am);

    
    
    /* ---------------------------------------------------------------------- */
    /* return to MATLAB */
    /* ---------------------------------------------------------------------- */
   
    /* return minor (translate to MATLAB convention) */
    if (nargout > 1)
    {
	pargout [1] = mxCreateDoubleMatrix (1, 1, mxREAL) ;
	px = mxGetPr (pargout [1]) ;
	px [0] = ((minor == n) ? 0 : (minor+1)) ;
    }

    /* return permutation */
    if (nargout > 2)
    {
	pargout [2] = sputil_put_int (L->Perm, n, 1) ;
    }

    /* ---------------------------------------------------------------------- */
    /* free workspace and the CHOLMOD L, except for what is copied to MATLAB */
    /* ---------------------------------------------------------------------- */

    cholmod_l_free_factor (&L, cm) ;
    cholmod_l_finish (cm) ;
    cholmod_l_print_common (" ", cm) ;
    /*
    if (cm->malloc_count != 3 + mxIsComplex (pargout[0])) mexErrMsgTxt ("!") ;
    */
}

void cumsum2 (mwIndex *p, mwIndex *c, int n)
{
  int i;
  mwIndex nz = 0;
  if(!p || !c) return;
  for (i=0;i<n;i++){
    p[i]=nz;
    nz+=c[i];
    c[i]=p[i];
  }
  p[n]=nz;
}
