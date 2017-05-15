#ifndef __ACONSTANTS_H

#define __ACONSTANTS_H 1

#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <string>
#include <iomanip>

#define PI 3.14159265358979323846
#define SQRT3 1.73205080756887729353

/// tolerance to check if something is zero, ie, macine epsilon
#define ZERO_TOL 2.2e-16

/// A small number likely smaller than most convergence tolerances
#define A_SMALL_NUMBER 1e-12

#define NDIM 2
#define NVARS 4
#define NGAUSS 1

#ifndef MESHDATA_DOUBLE_PRECISION
#define MESHDATA_DOUBLE_PRECISION 20
#endif

namespace acfd
{
	typedef double a_real;
	typedef int a_int;
}

#ifndef EIGEN_CORE_H
#include <Eigen/Core>
namespace Eigen
{
	typedef Matrix<acfd::a_real, Dynamic, Dynamic, RowMajor> Matrix;
	typedef Matrix<acfd::a_real, Dynamic, 1> Vector;
}
#endif
