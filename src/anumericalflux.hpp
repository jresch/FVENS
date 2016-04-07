/** \file anumericalflux.hpp
 * \brief Implements numerical flux schemes for Euler and Navier-Stokes equations.
 * \author Aditya Kashi
 * \date March 2015
 */

#ifndef __ACONSTANTS_H
#include <aconstants.hpp>
#endif

#ifndef __AMATRIX2_H
#include <amatrix2.hpp>
#endif

#ifndef __AMESH2D_H
#include <amesh2.hpp>
#endif

//#include "aquadrature.hpp"
#include "alimiter.hpp"

#define __ANUMERICALFLUX_H 1

namespace acfd {

/// Adiabatic index
const double g = 1.4;

/// Abstract class from which to derive all numerical flux classes
/** The class is such that given the left and right states and a face normal, the numerical flux is computed.
 */
class InviscidFlux
{
protected:
	int nvars;		///< Number of conserved variables
	int ndim;		///< Number of spatial dimensions involved
	acfd_real g;	///< Adiabatic index

public:
	/// Sets up data for the inviscid flux scheme
	InviscidFlux(int num_vars, int num_dims, acfd_real gamma);

	/** Computes flux across a face with
	 * \param[in] uleft is the vector of left states for the face
	 * \param[in] uright is the vector of right states for the face
	 * \param[in] n is the normal vector to the face
	 * \param[in|out] flux contains the computed flux
	 */
	virtual void get_flux(const amat::Matrix<acfd_real>* const uleft, const amat::Matrix<acfd_real>* const uright, const acfd_real* const n, amat::Matrix<acfd_real>* const flux) = 0;
};

InviscidFlux::InviscidFlux(int num_vars, int num_dims, acfd_real gamma) : nvars(num_vars), ndim(num_dims), g(gamma)
{ }

/// Given left and right states at each face, the Van-Leer flux-vector-splitting is calculated at each face
class VanLeerFlux : public InviscidFlux
{
	amat::Matrix<double> fiplus;
	amat::Matrix<double> fjminus;

public:
	VanLeerFlux(int num_vars, int num_dims, acfd_real gamma);
	void get_flux(const amat::Matrix<acfd_real>* const ul, const amat::Matrix<acfd_real>* const ur, const acfd_real* const n, amat::Matrix<acfd_real>* const flux);
};

VanLeerFlux::VanLeerFlux(int num_vars, int num_dims, acfd_real gamma) : InviscidFlux(num_vars, num_dims, gamma)
{
	//InviscidFlux::setup(mesh, uleft, uright, rhs_ele, cfl_denom);
	fiplus.setup(nvars, 1);
	fjminus.setup(nvars, 1);
}

void VanLeerFlux::get_flux(const amat::Matrix<acfd_real>* ul, const amat::Matrix<acfd_real>* ur, const acfd_real* const n, amat::Matrix<acfd_real>* const flux)
{
	acfd_real nx, ny, pi, ci, vni, Mni, pj, cj, vnj, Mnj, vmags;

	nx = n[0];
	ny = n[1];

	//calculate presures from u
	pi = (g-1)*(ul->get(3) - 0.5*(pow(ul->get(1),2)+pow(ul->get(2),2))/ul->get(0));
	pj = (g-1)*(ur->get(3) - 0.5*(pow(ur->get(1),2)+pow(ur->get(2),2))/ur->get(0));
	//calculate speeds of sound
	ci = sqrt(g*pi/ul->get(0));
	cj = sqrt(g*pj/ur->get(0));
	//calculate normal velocities
	vni = (ul->get(1)*nx +ul->get(2)*ny)/ul->get(0);
	vnj = (ur->get(1)*nx + ur->get(2)*ny)/ur->get(0);

	//Normal mach numbers
	Mni = vni/ci;
	Mnj = vnj/cj;

	//Calculate split fluxes
	if(Mni < -1.0) fiplus.zeros();
	else if(Mni > 1.0)
	{
		fiplus(0) = ul->get(0)*vni;
		fiplus(1) = vni*ul->get(1) + pi*nx;
		fiplus(2) = vni*ul->get(2) + pi*ny;
		fiplus(3) = vni*(ul->get(3) + pi);
	}
	else
	{
		vmags = pow(ul->get(1)/ul->get(0), 2) + pow(ul->get(2)/ul->get(0), 2);	// square of velocity magnitude
		fiplus(0) = ul->get(0)*ci*pow(Mni+1, 2)/4.0;
		fiplus(1) = fiplus(0) * (ul->get(1)/ul->get(0) + nx*(2.0*ci - vni)/g);
		fiplus(2) = fiplus(0) * (ul->get(2)/ul->get(0) + ny*(2.0*ci - vni)/g);
		fiplus(3) = fiplus(0) * ( (vmags - vni*vni)/2.0 + pow((g-1)*vni+2*ci, 2)/(2*(g*g-1)) );
	}

	if(Mnj > 1.0) fjminus.zeros();
	else if(Mnj < -1.0)
	{
		fjminus(0) = ur->get(0)*vnj;
		fjminus(1) = vnj*ur->get(1) + pj*nx;
		fjminus(2) = vnj*ur->get(2) + pj*ny;
		fjminus(3) = vnj*(ur->get(3) + pj);
	}
	else
	{
		vmags = pow(ur->get(1)/ur->get(0), 2) + pow(ur->get(2)/ur->get(0), 2);	// square of velocity magnitude
		fjminus(0) = -ur->get(0)*cj*pow(Mnj-1, 2)/4.0;
		fjminus(1) = fjminus(0) * (ur->get(1)/ur->get(0) + nx*(-2.0*cj - vnj)/g);
		fjminus(2) = fjminus(0) * (ur->get(2)/ur->get(0) + ny*(-2.0*cj - vnj)/g);
		fjminus(3) = fjminus(0) * ( (vmags - vnj*vnj)/2.0 + pow((g-1)*vnj-2*cj, 2)/(2*(g*g-1)) );
	}

	//Update the flux vector
	for(int i = 0; i < nvars; i++)
		(*flux)(i) = (fiplus(i) + fjminus(i));
}

/// Roe flux-difference splitting Riemann solver for the Euler equations
class RoeFlux : public InviscidFlux
{
public:
	RoeFlux(int num_vars, int num_dims, acfd_real gamma);
	void get_flux(const amat::Matrix<acfd_real>* const uleft, const amat::Matrix<acfd_real>* const uright, const acfd_real* const n, amat::Matrix<acfd_real>* const flux);
};

RoeFlux::RoeFlux(int num_vars, int num_dims, acfd_real gamma) : InviscidFlux(num_vars, num_dims, gamma)
{ }

void RoeFlux::get_flux(const amat::Matrix<acfd_real>* const ul, const amat::Matrix<acfd_real>* const ur, const acfd_real* const n, amat::Matrix<acfd_real>* const flux)
{
	acfd_real Hi, Hj, ci, cj, pi, pj, vxi, vxj, vyi, vyj, vmag2i, vmag2j, vni, vnj;
	int ivar;

	vxi = ul->get(1)/ul->get(0); vyi = ul->get(2)/ul->get(0);
	vxj = ur->get(1)/ur->get(0); vyj = ur->get(2)/ur->get(0);
	vni = vxi*n[0] + vyi*n[1];
	vnj = vxj*n[0] + vyj*n[1];
	vmag2i = vxi*vxi + vyi*vyi;
	vmag2j = vxj*vxj + vyj*vyj;
	// pressures
	pi = (g-1.0)*(ul->get(3) - 0.5*ul->get(0)*vmag2i);
	pj = (g-1.0)*(ur->get(3) - 0.5*ur->get(0)*vmag2j);
	// speeds of sound
	ci = sqrt(g*pi/ul->get(0));
	cj = sqrt(g*pj/ur->get(0));
	// enthalpies (E + p/rho = u(3)/u(0) + p/u(0)
	Hi = (ul->get(3) + pi)/ul->get(0);
	Hj = (ur->get(3) + pj)/ur->get(0);

	// compute Roe-averages
	
	acfd_real Rij, rhoij, vxij, vyij, Hij, cij, vm2ij, vnij;
	Rij = sqrt(ur->get(0)/ul->get(0));
	rhoij = Rij*ul->get(0);
	vxij = (Rij*vxj + vxi)/(Rij + 1.0);
	vyij = (Rij*vyj + vyi)/(Rij + 1.0);
	Hij = (Rij*Hj + Hi)/(Rij + 1.0);
	vm2ij = vxij*vxij + vyij*vyij;
	vnij = vxij*n[0] + vyij*n[1];
	cij = sqrt( (g-1.0)*(Hij - vm2ij*0.5) );

	// eigenvalues
	acfd_real l[4];
	l[0] = vnij; l[1] = vnij; l[2] = vnij + cij; l[3] = vnij - cij;

	// Harten-Hyman entropy fix
	acfd_real eps = 0;
	if(eps < l[0]-vni) eps = l[0]-vni;
	if(eps < vnj-l[0]) eps = vnj-l[0];
	if(fabs(l[0]) < eps) l[0] = eps;
	if(fabs(l[1]) < eps) l[1] = eps;

	eps = 0;
	if(eps < l[2]-(vni+ci)) eps = l[2]-(vni+ci);
	if(eps < vnj+cj - l[2]) eps = vnj+cj - l[2];
	if(fabs(l[2]) < eps) l[2] = eps;

	eps = 0;
	if(eps < l[3] - (vni-ci)) eps = l[3] - (vni-ci);
	if(eps < vnj-cj - l[3]) eps = vnj-cj - l[3];
	if(fabs(l[3]) < eps) l[3] = eps;

	// eigenvectors (column vectors of r below)
	amat::Matrix<acfd_real> r(4,4, amat::ROWMAJOR);

	r(0,0) = 1.0;		r(0,1) = 0;							r(0,2) = 1.0;				r(0,3) = 1.0;
	r(1,0) = vxij;		r(1,1) = cij*n[1];					r(1,2) = vxij + cij*n[0];	r(1,3) = vxij - cij*n[0];
	r(2,0) = vyij;		r(2,1) = -cij*n[0];					r(2,2) = vyij + cij*n[1];	r(2,3) = vyij - cij*n[1];
	r(3,0)= vm2ij*0.5;	r(3,1) = cij*(vxij*n[1]-vyij*n[0]);	r(3,2) = Hij + cij*vnij;	r(3,3) = Hij - cij*vnij;

	for(ivar = 0; ivar < 4; ivar++)
	{
		r(ivar,2) *= rhoij/(2.0*cij);
		r(ivar,3) *= rhoij/(2.0*cij);
	}

	// R^(-1)(qR-qL)
	amat::Matrix<acfd_real> dw(4,1);
	dw(0) = (ur->get(0)-ul->get(0)) - (pj-pi)/(cij*cij);
	dw(1) = (vxj-vxi)*n[1] - (vyj-vyi)*n[0];
	dw(2) = vnj-vni + (pj-pi)/(rhoij*cij);
	dw(3) = -(vnj-vni) + (pj-pi)/(rhoij*cij);

	// get one-sided flux vectors
	acfd_real fi[4], fj[4];
	fi[0] = ul->get(0)*vni;						fj[0] = ur->get(0)*vnj;
	fi[1] = ul->get(0)*vni*vxi + pi*n[0];		fj[1] = ur->get(0)*vnj*vxj + pj*n[0];
	fj[2] = ul->get(0)*vni*vyi + pi*n[1];		fj[2] = ur->get(0)*vnj*vyj + pj*n[1];
	fj[3] = vni*(ul->get(3) + pi);				fj[3] = vnj*(ur->get(3) + pj);

	// finally compute fluxes
	acfd_real sum; int j;
	for(ivar = 0; ivar < 4; ivar++)
	{
		sum = 0;
		for(j = 0; j < 4; j++)
			sum += fabs(l[j])*dw.get(j)*r.get(ivar,j);
		(*flux)(ivar) = 0.5*(fi[ivar]+fj[ivar] - sum);
	}
}

} // end namespace acfd
