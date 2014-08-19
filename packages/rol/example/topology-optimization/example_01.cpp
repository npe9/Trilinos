// @HEADER
// ************************************************************************
//
//               Rapid Optimization Library (ROL) Package
//                 Copyright (2014) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact lead developers:
//              Drew Kouri   (dpkouri@sandia.gov) and
//              Denis Ridzal (dridzal@sandia.gov)
//
// ************************************************************************
// @HEADER

/*! \file  example_01.cpp
    \brief Shows how to solve a topology optimization problem using Moreau-Yoshida 
           regularization for the volume constraint.
*/

#include "ROL_Algorithm.hpp"
#include "ROL_CompositeStepSQP.hpp"
#include "ROL_TrustRegionStep.hpp"
#include "ROL_LineSearchStep.hpp"
#include "ROL_StatusTest.hpp"
#include "ROL_Types.hpp"
#include "Teuchos_oblackholestream.hpp"
#include "Teuchos_GlobalMPISession.hpp"
#include "Teuchos_XMLParameterListHelpers.hpp"
#include "Teuchos_LAPACK.hpp"

#include <iostream>
#include <algorithm>

#include "ROL_StdVector.hpp"
#include "ROL_Vector_SimOpt.hpp"
#include "ROL_EqualityConstraint_SimOpt.hpp"
#include "ROL_Objective_SimOpt.hpp"
#include "ROL_Reduced_Objective_SimOpt.hpp"
#include "ROL_StdBoundConstraint.hpp"

#include "Teuchos_SerialDenseVector.hpp"
#include "Teuchos_SerialDenseSolver.hpp"

template<class Real>
class FEM {
private:
  int nx_;
  int ny_;
  int p_;
  int prob_;
  Real E_;
  Real nu_;
  Teuchos::SerialDenseMatrix<int,Real> KE_;

public:

  FEM(int nx, int ny, int p = 3, int prob = 1, Real E = 1.0, Real nu = 0.3) 
  : nx_(nx), ny_(ny), p_(p), prob_(prob), E_(E), nu_(nu) {
    std::vector<Real> k(8,0.0);
    k[0] =  1.0/2.0-nu_/6.0; 
    k[1] =  1.0/8.0+nu_/8.0;
    k[2] = -1.0/4.0-nu_/12.0;
    k[3] = -1.0/8.0+3.0*nu_/8.0;
    k[4] = -1.0/4.0+nu_/12.0;
    k[5] = -1.0/8.0-nu_/8.0;
    k[6] = nu_/6.0;
    k[7] =  1.0/8.0-3.0*nu_/8.0;
    KE_.shape(8,8);
    // Row 1
    KE_(0,0) = E_/(1.0-nu_*nu_)*k[0];
    KE_(0,1) = E_/(1.0-nu_*nu_)*k[1];
    KE_(0,2) = E_/(1.0-nu_*nu_)*k[2];
    KE_(0,3) = E_/(1.0-nu_*nu_)*k[3];
    KE_(0,4) = E_/(1.0-nu_*nu_)*k[4];
    KE_(0,5) = E_/(1.0-nu_*nu_)*k[5];
    KE_(0,6) = E_/(1.0-nu_*nu_)*k[6];
    KE_(0,7) = E_/(1.0-nu_*nu_)*k[7];
    // Row 2
    KE_(1,0) = E_/(1.0-nu_*nu_)*k[1];
    KE_(1,1) = E_/(1.0-nu_*nu_)*k[0];
    KE_(1,2) = E_/(1.0-nu_*nu_)*k[7];
    KE_(1,3) = E_/(1.0-nu_*nu_)*k[6];
    KE_(1,4) = E_/(1.0-nu_*nu_)*k[5];
    KE_(1,5) = E_/(1.0-nu_*nu_)*k[4];
    KE_(1,6) = E_/(1.0-nu_*nu_)*k[3];
    KE_(1,7) = E_/(1.0-nu_*nu_)*k[2];
    // Row 3
    KE_(2,0) = E_/(1.0-nu_*nu_)*k[2];
    KE_(2,1) = E_/(1.0-nu_*nu_)*k[7];
    KE_(2,2) = E_/(1.0-nu_*nu_)*k[0];
    KE_(2,3) = E_/(1.0-nu_*nu_)*k[5];
    KE_(2,4) = E_/(1.0-nu_*nu_)*k[6];
    KE_(2,5) = E_/(1.0-nu_*nu_)*k[3];
    KE_(2,6) = E_/(1.0-nu_*nu_)*k[4];
    KE_(2,7) = E_/(1.0-nu_*nu_)*k[1];
    // Row 4
    KE_(3,0) = E_/(1.0-nu_*nu_)*k[3];
    KE_(3,1) = E_/(1.0-nu_*nu_)*k[6];
    KE_(3,2) = E_/(1.0-nu_*nu_)*k[5];
    KE_(3,3) = E_/(1.0-nu_*nu_)*k[0];
    KE_(3,4) = E_/(1.0-nu_*nu_)*k[7];
    KE_(3,5) = E_/(1.0-nu_*nu_)*k[2];
    KE_(3,6) = E_/(1.0-nu_*nu_)*k[1];
    KE_(3,7) = E_/(1.0-nu_*nu_)*k[4];
    // Row 5
    KE_(4,0) = E_/(1.0-nu_*nu_)*k[4];
    KE_(4,1) = E_/(1.0-nu_*nu_)*k[5];
    KE_(4,2) = E_/(1.0-nu_*nu_)*k[6];
    KE_(4,3) = E_/(1.0-nu_*nu_)*k[7];
    KE_(4,4) = E_/(1.0-nu_*nu_)*k[0];
    KE_(4,5) = E_/(1.0-nu_*nu_)*k[1];
    KE_(4,6) = E_/(1.0-nu_*nu_)*k[2];
    KE_(4,7) = E_/(1.0-nu_*nu_)*k[3];
    // Row 6
    KE_(5,0) = E_/(1.0-nu_*nu_)*k[5];
    KE_(5,1) = E_/(1.0-nu_*nu_)*k[4];
    KE_(5,2) = E_/(1.0-nu_*nu_)*k[3];
    KE_(5,3) = E_/(1.0-nu_*nu_)*k[2];
    KE_(5,4) = E_/(1.0-nu_*nu_)*k[1];
    KE_(5,5) = E_/(1.0-nu_*nu_)*k[0];
    KE_(5,6) = E_/(1.0-nu_*nu_)*k[7];
    KE_(5,7) = E_/(1.0-nu_*nu_)*k[6];
    // Row 7
    KE_(6,0) = E_/(1.0-nu_*nu_)*k[6];
    KE_(6,1) = E_/(1.0-nu_*nu_)*k[3];
    KE_(6,2) = E_/(1.0-nu_*nu_)*k[4];
    KE_(6,3) = E_/(1.0-nu_*nu_)*k[1];
    KE_(6,4) = E_/(1.0-nu_*nu_)*k[2];
    KE_(6,5) = E_/(1.0-nu_*nu_)*k[7];
    KE_(6,6) = E_/(1.0-nu_*nu_)*k[0];
    KE_(6,7) = E_/(1.0-nu_*nu_)*k[5];
    // Row 8
    KE_(7,0) = E_/(1.0-nu_*nu_)*k[7];
    KE_(7,1) = E_/(1.0-nu_*nu_)*k[2];
    KE_(7,2) = E_/(1.0-nu_*nu_)*k[1];
    KE_(7,3) = E_/(1.0-nu_*nu_)*k[4];
    KE_(7,4) = E_/(1.0-nu_*nu_)*k[3];
    KE_(7,5) = E_/(1.0-nu_*nu_)*k[6];
    KE_(7,6) = E_/(1.0-nu_*nu_)*k[5];
    KE_(7,7) = E_/(1.0-nu_*nu_)*k[0];
  }

  int numZ(void) { return this->nx_*this->ny_; }
  int numU(void) { return 2*(this->nx_+1)*(this->ny_+1); }
  int power(void) { return this->p_; }

  int get_index(int r, int n1, int n2) {
    int ind = 0;
    switch(r) {
      case 0: ind = 2*n1-2; break;
      case 1: ind = 2*n1-1; break;
      case 2: ind = 2*n2-2; break;
      case 3: ind = 2*n2-1; break;
      case 4: ind = 2*n2;   break;
      case 5: ind = 2*n2+1; break;
      case 6: ind = 2*n1;   break;
      case 7: ind = 2*n1+1; break;
    }
    return ind;
  }

  bool check_on_boundary(int r) {
    switch(this->prob_) {
      case 0: 
        if ( (r < 2*(this->ny_+1) && r%2 == 0) || ( r == 2*(this->nx_+1)*(this->ny_+1)-1 ) ) {
          return true;
        }
        break;
      case 1: 
        if ( r < 2*(this->ny_+1) ) {
          return true;
        }
        break;
    }
    return false;
  }

  void set_boundary_conditions(Teuchos::SerialDenseVector<int, Real> &U) {
    for (int i=0; i<U.length(); i++) {
      if ( this->check_on_boundary(i) ) {
        U(i) = 0.0;
      }
    }
  }

  void set_boundary_conditions(std::vector<Real> &u) {
    for (int i=0; i<u.size(); i++) {
      if ( this->check_on_boundary(i) ) {
        u[i] = 0.0;
      }
    }
  }

  void build_force(std::vector<Real> &F) {
    F.assign(this->numU(),0.0);
    switch(this->prob_) {
      case 0: F[1] = -1;                               break;
      case 1: F[2*(this->nx_+1)*(this->ny_+1)-1] = -1; break;
    }
  }

  void build_force(Teuchos::SerialDenseVector<int, Real> &F) {
    F.resize(this->numU());
    F.putScalar(0.0);
    switch(this->prob_) {
      case 0: F(1) = -1;                               break;
      case 1: F(2*(this->nx_+1)*(this->ny_+1)-1) = -1; break;
    }
  }

  void build_jacobian(Teuchos::SerialDenseMatrix<int, Real> &K, const std::vector<Real> &z, 
                      bool transpose = false) {
    // Fill jacobian
    K.shape(2*(this->nx_+1)*(this->ny_+1),2*(this->nx_+1)*(this->ny_+1));
    int n1 = 0, n2 = 0, row = 0, col = 0;
    Real Zp = 0.0, val = 0.0;
    for (int i=0; i<this->nx_; i++) {
      for (int j=0; j<this->ny_; j++) {
        n1 = (this->ny_+1)* i   +(j+1);
        n2 = (this->ny_+1)*(i+1)+(j+1); 
        Zp = std::pow(z[i+j*this->nx_],(Real)this->p_);
        for (int r=0; r<8; r++) {
          row = this->get_index(r,n1,n2); 
          if ( this->check_on_boundary(row) ) {
            K(row,row) = 1.0;
          }
          else {
            for (int c=0; c<8; c++) {
              col = this->get_index(c,n1,n2);
              if ( !this->check_on_boundary(col) ) {
                val = Zp*(this->KE_)(r,c);
                if (transpose) {
                  K(col,row) += val;
                }
                else {
                  K(row,col) += val;
                }
              }
            }
          }
        }
      }
    }
  } 

  void build_jacobian(Teuchos::SerialDenseMatrix<int, Real> &K, const std::vector<Real> &z, 
                      const std::vector<Real> &v, bool transpose = false) {
    // Fill jacobian
    K.shape(2*(this->nx_+1)*(this->ny_+1),2*(this->nx_+1)*(this->ny_+1));
    int n1 = 0, n2 = 0, row = 0, col = 0;
    Real Zp = 0.0, V = 0.0, val = 0.0;
    for (int i=0; i<this->nx_; i++) {
      for (int j=0; j<this->ny_; j++) {
        n1 = (this->ny_+1)* i   +(j+1);
        n2 = (this->ny_+1)*(i+1)+(j+1); 
        Zp = (this->p_ == 1) ? 1.0 : (Real)this->p_*std::pow(z[i+j*this->nx_],(Real)this->p_-1.0);
        V  = v[i+j*this->nx_];
        for (int r=0; r<8; r++) {
          row = this->get_index(r,n1,n2); 
          if ( this->check_on_boundary(row) ) {
            K(row,row) = 1.0;
          }
          else {
            for (int c=0; c<8; c++) {
              col = this->get_index(c,n1,n2);
              if ( !this->check_on_boundary(col) ) {
                val = Zp*V*(this->KE_)(r,c);
                if (transpose) {
                  K(col,row) += val;
                }
                else {
                  K(row,col) += val;
                }
              }
            }
          }
        }
      }
    }
  }

  void apply_jacobian(std::vector<Real> &ju, const std::vector<Real> &u, const std::vector<Real> &z) {
    int n1 = 0, n2 = 0, row = 0, col = 0;
    Real Zp = 0.0;
    ju.assign(u.size(),0.0);
    for (int i=0; i<this->nx_; i++) {
      for (int j=0; j<this->ny_; j++) {
        n1 = (this->ny_+1)* i   +(j+1);
        n2 = (this->ny_+1)*(i+1)+(j+1); 
        Zp = std::pow(z[i+j*this->nx_],(Real)this->p_);
        for (int r=0; r<8; r++) {
          row = this->get_index(r,n1,n2); 
          if ( this->check_on_boundary(row) ) {
            ju[row] = u[row];
          }
          else {
            for (int c=0; c<8; c++) {
              col = this->get_index(c,n1,n2);
              if ( !this->check_on_boundary(col) ) {
                ju[row] += Zp*(this->KE_)(r,c)*u[col];
              }
            }
          }
        }
      }
    }
  } 

  void apply_jacobian(std::vector<Real> &ju, const std::vector<Real> &u, const std::vector<Real> &z, 
                      const std::vector<Real> &v) {
    // Fill jacobian
    int n1 = 0, n2 = 0, row = 0, col = 0;
    Real Zp = 0.0, V = 0.0;
    ju.assign(u.size(),0.0);
    for (int i=0; i<this->nx_; i++) {
      for (int j=0; j<this->ny_; j++) {
        n1 = (this->ny_+1)* i   +(j+1);
        n2 = (this->ny_+1)*(i+1)+(j+1); 
        Zp = (this->p_ == 1) ? 1.0 : (Real)this->p_*std::pow(z[i+j*this->nx_],(Real)this->p_-1.0);
        V  = v[i+j*this->nx_];
        for (int r=0; r<8; r++) {
          row = get_index(r,n1,n2); 
          if ( this->check_on_boundary(row) ) {
            ju[row] = u[row];
          }
          else {
            for (int c=0; c<8; c++) {
              col = get_index(c,n1,n2);
              if ( !this->check_on_boundary(col) ) {
                ju[row] += V*Zp*(this->KE_)(r,c)*u[col];
              }
            }
          }
        }
      }
    }
  } 

  void apply_adjoint_jacobian(std::vector<Real> &jv, const std::vector<Real> &u, const std::vector<Real> &z, 
                              const std::vector<Real> &v) {
    // Fill jacobian
    int n1 = 0, n2 = 0, row = 0, col = 0;
    Real Zp = 0.0, VKU = 0.0;
    for (int i=0; i<this->nx_; i++) {
      for (int j=0; j<this->ny_; j++) {
        n1 = (this->ny_+1)* i   +(j+1);
        n2 = (this->ny_+1)*(i+1)+(j+1); 
        Zp = (this->p_ == 1) ? 1.0 : (Real)this->p_*std::pow(z[i+j*this->nx_],(Real)this->p_-1.0);
        VKU = 0.0;
        for (int r=0; r<8; r++) {
          row = get_index(r,n1,n2); 
          if ( this->check_on_boundary(row) ) {
            VKU += v[row]*u[row];
          }
          else {
            for (int c=0; c<8; c++) {
              col = get_index(c,n1,n2);
              if ( !this->check_on_boundary(col) ) {
                VKU += Zp*(this->KE_)(r,c)*u[col]*v[row];
              }
            }
          }
        }
        jv[i+j*this->nx_] = VKU;
      }
    }
  }

  void apply_adjoint_jacobian(std::vector<Real> &jv, const std::vector<Real> &u, const std::vector<Real> &z, 
                              const std::vector<Real> &v, const std::vector<Real> &w) {
    // Fill jacobian
    int n1 = 0, n2 = 0, row = 0, col = 0;
    Real Zp = 0.0, V = 0.0, VKU = 0.0;
    for (int i=0; i<this->nx_; i++) {
      for (int j=0; j<this->ny_; j++) {
        n1 = (this->ny_+1)* i   +(j+1);
        n2 = (this->ny_+1)*(i+1)+(j+1); 
        Zp = (this->p_ == 1) ? 0.0 : 
               ((this->p_ == 2) ? 2.0 : 
                 (Real)this->p_*((Real)this->p_-1.0)*std::pow(z[i+j*this->nx_],(Real)this->p_-2.0));
        V  = v[i+j*this->nx_];
        VKU = 0.0;
        for (int r=0; r<8; r++) {
          row = get_index(r,n1,n2); 
          if ( this->check_on_boundary(row) ) {
            VKU += w[row]*u[row];
          }
          else {
            for (int c=0; c<8; c++) {
              col = get_index(c,n1,n2);
              if ( !this->check_on_boundary(col) ) {
                VKU += Zp*V*(this->KE_)(r,c)*u[col]*w[row];
              }
            }
          }
        }
        jv[i+j*this->nx_] = VKU;
      }
    }
  }
};

template<class Real>
class EqualityConstraint_TopOpt : public ROL::EqualityConstraint_SimOpt<Real> {
private:
  Teuchos::RCP<FEM<Real> > FEM_;

public:

  EqualityConstraint_TopOpt(Teuchos::RCP<FEM<Real> > & FEM) : FEM_(FEM) {}

  void value(ROL::Vector<Real> &c, const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol) {
    Teuchos::RCP<std::vector<Real> > cp =
      Teuchos::rcp_const_cast<std::vector<Real> >((Teuchos::dyn_cast<ROL::StdVector<Real> >(c)).getVector());
    this->applyJacobian_1(c, u, u, z, tol);
    std::vector<Real> f;
    this->FEM_->build_force(f);
    for (int i = 0; i < f.size(); i++) {
      (*cp)[i] -= f[i];
    }
  }

  void solve(ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol) {
    Teuchos::RCP<std::vector<Real> > up =
      Teuchos::rcp_const_cast<std::vector<Real> >((Teuchos::dyn_cast<ROL::StdVector<Real> >(u)).getVector());
    Teuchos::RCP<const std::vector<Real> > zp =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(z))).getVector();
    // Assemble Jacobian
    Teuchos::SerialDenseMatrix<int, Real> K;
    this->FEM_->build_jacobian(K,*zp);
    // Assemble RHS
    Teuchos::SerialDenseVector<int, Real> F(K.numRows());
    this->FEM_->build_force(F);
    // Solve
    Teuchos::SerialDenseVector<int, Real> U(K.numCols());
    Teuchos::SerialDenseSolver<int, Real> solver;
    solver.setMatrix(Teuchos::rcp(&K, false));
    solver.setVectors(Teuchos::rcp(&U, false), Teuchos::rcp(&F, false));
    solver.factorWithEquilibration(true);
    solver.factor();
    solver.solve();
    this->FEM_->set_boundary_conditions(U);
    // Retrieve solution
    up->resize(U.length(),0.0);
    for (int i=0; i<U.length(); i++) {
      (*up)[i] = U(i);
    }
    // Compute residual
    //Teuchos::RCP<ROL::Vector<Real> > c = u.clone();
    //this->value(*c,u,z,tol);
    //std::cout << " IN SOLVE: ||c(u,z)|| = " << c->norm() << "\n";
  }

  void applyJacobian_1(ROL::Vector<Real> &jv, const ROL::Vector<Real> &v, const ROL::Vector<Real> &u, 
                       const ROL::Vector<Real> &z, Real &tol) {
    Teuchos::RCP<std::vector<Real> > jvp =
      Teuchos::rcp_const_cast<std::vector<Real> >((Teuchos::dyn_cast<ROL::StdVector<Real> >(jv)).getVector());
    Teuchos::RCP<const std::vector<Real> > vp =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(v))).getVector();
    Teuchos::RCP<const std::vector<Real> > zp =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(z))).getVector();
    // Apply Jacobian
    std::vector<Real> V;
    V.assign(vp->begin(),vp->end());
    this->FEM_->set_boundary_conditions(V);
    this->FEM_->apply_jacobian(*jvp,V,*zp);
  }

  void applyJacobian_2(ROL::Vector<Real> &jv, const ROL::Vector<Real> &v, const ROL::Vector<Real> &u,
                       const ROL::Vector<Real> &z, Real &tol) {
    Teuchos::RCP<std::vector<Real> > jvp =
      Teuchos::rcp_const_cast<std::vector<Real> >((Teuchos::dyn_cast<ROL::StdVector<Real> >(jv)).getVector());
    Teuchos::RCP<const std::vector<Real> > vp =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(v))).getVector();
    Teuchos::RCP<const std::vector<Real> > up =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(u))).getVector();
    Teuchos::RCP<const std::vector<Real> > zp =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(z))).getVector();
    // Apply Jacobian
    std::vector<Real> U;
    U.assign(up->begin(),up->end());
    this->FEM_->set_boundary_conditions(U);
    this->FEM_->apply_jacobian(*jvp,U,*zp,*vp);
  }

  void applyInverseJacobian_1(ROL::Vector<Real> &ijv, const ROL::Vector<Real> &v, const ROL::Vector<Real> &u,
                              const ROL::Vector<Real> &z, Real &tol) {
    Teuchos::RCP<std::vector<Real> > ijvp =
      Teuchos::rcp_const_cast<std::vector<Real> >((Teuchos::dyn_cast<ROL::StdVector<Real> >(ijv)).getVector());
    Teuchos::RCP<const std::vector<Real> > vp =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(v))).getVector();
    Teuchos::RCP<const std::vector<Real> > up =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(u))).getVector();
    Teuchos::RCP<const std::vector<Real> > zp =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(z))).getVector();
    // Assemble Jacobian
    Teuchos::SerialDenseMatrix<int, Real> K;
    this->FEM_->build_jacobian(K,*zp);
    // Solve
    Teuchos::SerialDenseVector<int, Real> U(K.numCols());
    Teuchos::SerialDenseVector<int, Real> F(vp->size());
    for (int i=0; i<vp->size(); i++) {
      F(i) = (*vp)[i];
    }
    Teuchos::SerialDenseSolver<int, Real> solver;
    solver.setMatrix(Teuchos::rcp(&K, false));
    solver.setVectors(Teuchos::rcp(&U, false), Teuchos::rcp(&F, false));
    solver.factorWithEquilibration(true);
    solver.factor();
    solver.solve();
    this->FEM_->set_boundary_conditions(U);
    // Retrieve solution
    ijvp->resize(U.length(),0.0);
    for (int i=0; i<U.length(); i++) {
      (*ijvp)[i] = U(i);
    }
  }

  void applyAdjointJacobian_1(ROL::Vector<Real> &ajv, const ROL::Vector<Real> &v, const ROL::Vector<Real> &u, 
                              const ROL::Vector<Real> &z, Real &tol) {
    Teuchos::RCP<std::vector<Real> > jvp =
      Teuchos::rcp_const_cast<std::vector<Real> >((Teuchos::dyn_cast<ROL::StdVector<Real> >(ajv)).getVector());
    Teuchos::RCP<const std::vector<Real> > vp =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(v))).getVector();
    Teuchos::RCP<const std::vector<Real> > up =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(u))).getVector();
    Teuchos::RCP<const std::vector<Real> > zp =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(z))).getVector();
    // apply jacobian
    std::vector<Real> V;
    V.assign(vp->begin(),vp->end());
    this->FEM_->set_boundary_conditions(V);
    this->FEM_->apply_jacobian(*jvp,V,*zp);
  }

  void applyAdjointJacobian_2(ROL::Vector<Real> &jv, const ROL::Vector<Real> &v, const ROL::Vector<Real> &u,
                              const ROL::Vector<Real> &z, Real &tol) {
    Teuchos::RCP<std::vector<Real> > jvp =
      Teuchos::rcp_const_cast<std::vector<Real> >((Teuchos::dyn_cast<ROL::StdVector<Real> >(jv)).getVector());
    Teuchos::RCP<const std::vector<Real> > vp =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(v))).getVector();
    Teuchos::RCP<const std::vector<Real> > up =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(u))).getVector();
    Teuchos::RCP<const std::vector<Real> > zp =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(z))).getVector();
    // Apply Jacobian
    std::vector<Real> U;
    U.assign(up->begin(),up->end());
    this->FEM_->set_boundary_conditions(U);
    std::vector<Real> V;
    V.assign(vp->begin(),vp->end());
    this->FEM_->set_boundary_conditions(V);
    this->FEM_->apply_adjoint_jacobian(*jvp,U,*zp,V);
  }

  void applyInverseAdjointJacobian_1(ROL::Vector<Real> &iajv, const ROL::Vector<Real> &v,
                                     const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol) {
    Teuchos::RCP<std::vector<Real> > iajvp =
      Teuchos::rcp_const_cast<std::vector<Real> >((Teuchos::dyn_cast<ROL::StdVector<Real> >(iajv)).getVector());
    Teuchos::RCP<const std::vector<Real> > vp =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(v))).getVector();
    Teuchos::RCP<const std::vector<Real> > up =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(u))).getVector();
    Teuchos::RCP<const std::vector<Real> > zp =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(z))).getVector();
    // Assemble Jacobian
    Teuchos::SerialDenseMatrix<int, Real> K;
    this->FEM_->build_jacobian(K,*zp);
    // Solve
    Teuchos::SerialDenseVector<int, Real> U(K.numCols());
    Teuchos::SerialDenseVector<int, Real> F(vp->size());
    for (int i=0; i<vp->size(); i++) {
      F(i) = (*vp)[i];
    }
    Teuchos::SerialDenseSolver<int, Real> solver;
    solver.setMatrix(Teuchos::rcp(&K, false));
    solver.setVectors(Teuchos::rcp(&U, false), Teuchos::rcp(&F, false));
    solver.factorWithEquilibration(true);
    solver.factor();
    solver.solve();
    this->FEM_->set_boundary_conditions(U);
    // Retrieve solution
    iajvp->resize(U.length(),0.0);
    for (int i=0; i<U.length(); i++) {
      (*iajvp)[i] = U(i);
    }
  }

  void applyAdjointHessian_11(ROL::Vector<Real> &ahwv, const ROL::Vector<Real> &w, const ROL::Vector<Real> &v,
                              const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol) {
    ahwv.zero();
  }
  
  void applyAdjointHessian_12(ROL::Vector<Real> &ahwv, const ROL::Vector<Real> &w, const ROL::Vector<Real> &v,
                              const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol) {
    this->applyAdjointJacobian_2(ahwv,w,v,z,tol);
  }
  void applyAdjointHessian_21(ROL::Vector<Real> &ahwv, const ROL::Vector<Real> &w, const ROL::Vector<Real> &v,
                              const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol) {
    this->applyJacobian_2(ahwv,v,w,z,tol);
  }
  void applyAdjointHessian_22(ROL::Vector<Real> &ahwv, const ROL::Vector<Real> &w, const ROL::Vector<Real> &v,
                              const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol) {
    Teuchos::RCP<std::vector<Real> > ahwvp =
      Teuchos::rcp_const_cast<std::vector<Real> >((Teuchos::dyn_cast<ROL::StdVector<Real> >(ahwv)).getVector());
    Teuchos::RCP<const std::vector<Real> > wp =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(w))).getVector();
    Teuchos::RCP<const std::vector<Real> > vp =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(v))).getVector();
    Teuchos::RCP<const std::vector<Real> > up =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(u))).getVector();
    Teuchos::RCP<const std::vector<Real> > zp =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(z))).getVector();
    // Apply Jacobian
    std::vector<Real> U;
    U.assign(up->begin(),up->end());
    this->FEM_->set_boundary_conditions(U);
    std::vector<Real> W;
    W.assign(wp->begin(),wp->end());
    this->FEM_->set_boundary_conditions(W);
    this->FEM_->apply_adjoint_jacobian(*ahwvp,U,*zp,*vp,W);
  }

  //void solveAugmentedSystem(ROL::Vector<Real> &v1, ROL::Vector<Real> &v2, const ROL::Vector<Real> &b1,
  //                          const ROL::Vector<Real> &b2, const ROL::Vector<Real> &x, Real &tol) {}
};

template<class Real>
class Objective_TopOpt : public ROL::Objective_SimOpt<Real> {
private:
  Teuchos::RCP<FEM<Real> > FEM_;
  Real frac_; 
  Real reg_; 
  Real pen_;

public:

  Objective_TopOpt(Teuchos::RCP<FEM<Real> > FEM, Real frac = 0.5, Real reg = 1.0, Real pen = 1.0 )
    : FEM_(FEM), frac_(frac), reg_(reg), pen_(pen) {}

  Real value( const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol ) {
    Teuchos::RCP<const std::vector<Real> > up =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(u))).getVector();
    Teuchos::RCP<const std::vector<Real> > zp =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(z))).getVector();
    // Apply Jacobian
    std::vector<Real> KU(up->size(),0.0);
    //std::vector<Real> U;
    //U.assign(up->begin(),up->end());
    //this->FEM_->set_boundary_conditions(U);
    //this->FEM_->apply_jacobian(KU,U,*zp);
    this->FEM_->build_force(KU);
    // Compliance
    Real c = 0.0;
    for (int i=0; i<up->size(); i++) {
      c += (*up)[i]*KU[i];
    }
    // Compute Moreau-Yoshida term
    Real vol = 0.0, r = 0.0;
    for (int i=0; i<zp->size(); i++) {
      vol += (*zp)[i]; 
    }
    vol  = (vol <= this->frac_*this->FEM_->numZ()) ? 0.0 : (vol - this->frac_*this->FEM_->numZ());
    r = (this->reg_)*std::pow(vol,3.0);
    // Compute 0-1 penalty
    Real val = 0.0, p = 0.0;
    for (int i=0; i<zp->size(); i++) {
      val += (*zp)[i]*(1.0-(*zp)[i]);
    }
    p = (this->pen_)/(this->FEM_->numZ())*val;
    return c + r + p;
  }

  void gradient_1( ROL::Vector<Real> &g, const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol ) {
    // Unwrap g
    Teuchos::RCP<std::vector<Real> > gp = Teuchos::rcp_const_cast<std::vector<Real> >(
      (Teuchos::dyn_cast<const ROL::StdVector<Real> >(g)).getVector());
    // Unwrap x
    Teuchos::RCP<const std::vector<Real> > up =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(u))).getVector();
    Teuchos::RCP<const std::vector<Real> > zp =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(z))).getVector();
    // Apply Jacobian
    std::vector<Real> KU(up->size(),0.0);
    //std::vector<Real> U;
    //U.assign(up->begin(),up->end());
    //this->FEM_->set_boundary_conditions(U);
    //this->FEM_->apply_jacobian(KU,U,*zp);
    this->FEM_->build_force(KU);
    // Apply jacobian to u
    for (int i=0; i<up->size(); i++) {
      //(*gp)[i] = 2.0*KU[i];
      (*gp)[i] = KU[i];
    }
  }

  void gradient_2( ROL::Vector<Real> &g, const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol ) {
    // Unwrap g
    Teuchos::RCP<std::vector<Real> > gp = Teuchos::rcp_const_cast<std::vector<Real> >(
      (Teuchos::dyn_cast<const ROL::StdVector<Real> >(g)).getVector());
    // Unwrap x
    Teuchos::RCP<const std::vector<Real> > up =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(u))).getVector();
    Teuchos::RCP<const std::vector<Real> > zp =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(z))).getVector();
    // Apply Jacobian
    //std::vector<Real> U;
    //U.assign(up->begin(),up->end());
    //this->FEM_->set_boundary_conditions(U);
    //this->FEM_->apply_adjoint_jacobian(*gp,U,*zp,U);
    // Compute Moreau-Yoshida term
    Real vol = 0.0;
    for (int i=0; i<zp->size(); i++) {
      vol += (*zp)[i]; 
    }
    vol = (vol <= this->frac_*this->FEM_->numZ()) ? 0.0 : (vol - this->frac_*this->FEM_->numZ());
    for (int i=0; i<zp->size(); i++) {
      (*gp)[i] += (this->reg_)*3.0*std::pow(vol,2.0);
      // Compute 0-1 penalty
      (*gp)[i] += (this->pen_)/(this->FEM_->numZ())*(1.0-2.0*(*zp)[i]);
    }
  }

  void hessVec_11( ROL::Vector<Real> &hv, const ROL::Vector<Real> &v, 
                   const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol ) {
    Teuchos::RCP<std::vector<Real> > hvp = Teuchos::rcp_const_cast<std::vector<Real> >(
      (Teuchos::dyn_cast<const ROL::StdVector<Real> >(hv)).getVector());
    // Unwrap v
    Teuchos::RCP<const std::vector<Real> > vp =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(v))).getVector();
    // Unwrap x
    Teuchos::RCP<const std::vector<Real> > up =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(u))).getVector();
    Teuchos::RCP<const std::vector<Real> > zp =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(z))).getVector();
    // Apply Jacobian
    //std::vector<Real> KV(vp->size(),0.0);
    //std::vector<Real> V;
    //V.assign(vp->begin(),vp->end());
    //this->FEM_->set_boundary_conditions(V);
    //this->FEM_->apply_jacobian(KV,V,*zp);
    //for (int i=0; i<vp->size(); i++) {
    //  (*hvp)[i] = 2.0*KV[i];
    //}
    hv.zero();
  }

  void hessVec_12( ROL::Vector<Real> &hv, const ROL::Vector<Real> &v, 
                   const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol ) {
    // Unwrap hv
    Teuchos::RCP<std::vector<Real> > hvp = Teuchos::rcp_const_cast<std::vector<Real> >(
      (Teuchos::dyn_cast<const ROL::StdVector<Real> >(hv)).getVector());
    // Unwrap v
    Teuchos::RCP<const std::vector<Real> > vp =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(v))).getVector();
    // Unwrap x
    Teuchos::RCP<const std::vector<Real> > up =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(u))).getVector();
    Teuchos::RCP<const std::vector<Real> > zp =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(z))).getVector();
    // Apply Jacobian
    //std::vector<Real> KU(up->size(),0.0);
    //std::vector<Real> U;
    //U.assign(up->begin(),up->end());
    //this->FEM_->set_boundary_conditions(U);
    //this->FEM_->apply_jacobian(KU,U,*zp,*vp);
    //for (int i=0; i<up->size(); i++) {
    //  (*hvp)[i] = 2.0*KU[i];
    //}
    hv.zero();
  }

  void hessVec_21( ROL::Vector<Real> &hv, const ROL::Vector<Real> &v, 
                   const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol ) {
    // Unwrap g
    Teuchos::RCP<std::vector<Real> > hvp = Teuchos::rcp_const_cast<std::vector<Real> >(
      (Teuchos::dyn_cast<const ROL::StdVector<Real> >(hv)).getVector());
    // Unwrap v
    Teuchos::RCP<const std::vector<Real> > vp =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(v))).getVector();
    // Unwrap x
    Teuchos::RCP<const std::vector<Real> > up =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(u))).getVector();
    Teuchos::RCP<const std::vector<Real> > zp =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(z))).getVector();
    // Apply Jacobian
    //std::vector<Real> U;
    //U.assign(up->begin(),up->end());
    //this->FEM_->set_boundary_conditions(U);
    //std::vector<Real> V;
    //V.assign(vp->begin(),vp->end());
    //this->FEM_->set_boundary_conditions(V);
    //this->FEM_->apply_adjoint_jacobian(*hvp,U,*zp,V);
    //for (int i=0; i<hvp->size(); i++) {
    //  (*hvp)[i] *= 2.0;
    //}
    hv.zero();
  }

  void hessVec_22( ROL::Vector<Real> &hv, const ROL::Vector<Real> &v, 
                   const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol ) {
    Teuchos::RCP<std::vector<Real> > hvp = Teuchos::rcp_const_cast<std::vector<Real> >(
      (Teuchos::dyn_cast<const ROL::StdVector<Real> >(hv)).getVector());
    // Unwrap v
    Teuchos::RCP<const std::vector<Real> > vp =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(v))).getVector();
    // Unwrap x
    Teuchos::RCP<const std::vector<Real> > up =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(u))).getVector();
    Teuchos::RCP<const std::vector<Real> > zp =
      (Teuchos::dyn_cast<ROL::StdVector<Real> >(const_cast<ROL::Vector<Real> &>(z))).getVector();
    // Apply Jacobian
    //std::vector<Real> U;
    //U.assign(up->begin(),up->end());
    //this->FEM_->set_boundary_conditions(U);
    //std::vector<Real> V;
    //V.assign(vp->begin(),vp->end());
    //this->FEM_->set_boundary_conditions(V);
    //this->FEM_->apply_adjoint_jacobian(*hvp,U,*zp,*vp,U);
    // Compute Moreau-Yoshida term
    Real vol = 0.0, vvol = 0.0;
    for (int i=0; i<zp->size(); i++) {
      vol  += (*zp)[i]; 
      vvol += (*vp)[i];
    }
    vol  = (vol <= this->frac_*this->FEM_->numZ()) ? 0.0 : (vol - this->frac_*this->FEM_->numZ());
    for (int i=0; i<zp->size(); i++) {
      (*hvp)[i] += (this->reg_)*6.0*vol*vvol; //(*vzp)[i];
      // Compute 0-1 penalty
      (*hvp)[i] -= (this->pen_)/(this->FEM_->numZ())*2.0*(*vp)[i];
    }
  }
};

typedef double RealT;

int main(int argc, char *argv[]) {

  Teuchos::GlobalMPISession mpiSession(&argc, &argv);

  // This little trick lets us print to std::cout only if a (dummy) command-line argument is provided.
  int iprint     = argc - 1;
  Teuchos::RCP<std::ostream> outStream;
  Teuchos::oblackholestream bhs; // outputs nothing
  if (iprint > 0)
    outStream = Teuchos::rcp(&std::cout, false);
  else
    outStream = Teuchos::rcp(&bhs, false);

  int errorFlag  = 0;

  // *** Example body.
  try {
    // FEM problem description.
    int prob = 1;  // prob = 0 is the MBB beam example, prob = 1 is the cantilever beam example.
    int nx   = 32; // Number of x-elements (60 for prob = 1, 32 for prob = 2).
    int ny   = 20; // Number of y-elements (20 for prob = 1, 20 for prob = 2).
    int P    = 1;  // SIMP penalization power.
    Teuchos::RCP<FEM<RealT> > pFEM = Teuchos::rcp(new FEM<RealT>(nx,ny,P,prob));
    // Objective function description.
    int   nreg = 10;       // # of Moreau-Yoshida parameter updates.
    int   npen = 3;        // # of penalty parameter updates.
    RealT frac = 0.4;      // Volume fraction.
    RealT reg  = 1.0;      // Moreau-Yoshida regularization parameter.
    RealT pen  = 1.0;      // 0-1 penalty parameter. 
    // Optimization parameters.
    bool derivCheck = false; // Derivative check flag.
    bool useTR      = false; // Use trust-region or line-search.
    RealT gtol      = 1e-4;  // Norm of gradient tolerance.
    RealT stol      = 1e-8;  // Norm of step tolerance.
    int   maxit     = 500;   // Maximum number of iterations.
    // Read optimization input parameter list.
    std::string filename = "input.xml";
    Teuchos::RCP<Teuchos::ParameterList> parlist = Teuchos::rcp( new Teuchos::ParameterList() );
    Teuchos::updateParametersFromXmlFile( filename, Teuchos::Ptr<Teuchos::ParameterList>(&*parlist) );
    // Initialize RCPs.
    Teuchos::RCP<ROL::Objective_SimOpt<RealT> >         pobj;   // Full objective.
    Teuchos::RCP<ROL::Reduced_Objective_SimOpt<RealT> > robj;   // Reduced objective.
    Teuchos::RCP<ROL::DefaultAlgorithm<RealT> >         algo;   // Optimization algorithm.
    Teuchos::RCP<ROL::Step<RealT> >                     step;   // Globalized step.
    Teuchos::RCP<ROL::StatusTest<RealT> >               status; // Status test.
    // Initialize equality constraint.
    Teuchos::RCP<ROL::EqualityConstraint_SimOpt<RealT> > pcon = 
      Teuchos::rcp(new EqualityConstraint_TopOpt<RealT>(pFEM));
    // Initialize bound constraints.
    std::vector<RealT> lo(pFEM->numZ(),1.e-3);
    std::vector<RealT> hi(pFEM->numZ(),1.0);
    ROL::StdBoundConstraint<RealT> bound(lo,hi);
    // Initialize control vector.
    Teuchos::RCP<std::vector<RealT> > z_rcp  = Teuchos::rcp( new std::vector<RealT> (pFEM->numZ(), frac) );
    ROL::StdVector<RealT> z(z_rcp);
    Teuchos::RCP<ROL::Vector<RealT> > zp  = Teuchos::rcp(&z,false);
    // Initialize state vector.
    Teuchos::RCP<std::vector<RealT> > u_rcp  = Teuchos::rcp( new std::vector<RealT> (pFEM->numU(), 0.0) );
    ROL::StdVector<RealT> u(u_rcp);
    Teuchos::RCP<ROL::Vector<RealT> > up  = Teuchos::rcp(&u,false);
    // Initialize adjoint vector.
    Teuchos::RCP<std::vector<RealT> > p_rcp  = Teuchos::rcp( new std::vector<RealT> (pFEM->numU(), 0.0) );
    ROL::StdVector<RealT> p(p_rcp);
    Teuchos::RCP<ROL::Vector<RealT> > pp  = Teuchos::rcp(&p,false);
    // Derivative check.
    if (derivCheck) {
      // Initialize control vectors.
      Teuchos::RCP<std::vector<RealT> > yz_rcp = Teuchos::rcp( new std::vector<RealT> (pFEM->numZ(), 0.0) );
      for (int i=0; i<pFEM->numZ(); i++) {
        (*yz_rcp)[i] = frac * (RealT)rand()/(RealT)RAND_MAX;
      }
      ROL::StdVector<RealT> yz(yz_rcp);
      Teuchos::RCP<ROL::Vector<RealT> > yzp = Teuchos::rcp(&yz,false);
      // Initialize state vectors.
      Teuchos::RCP<std::vector<RealT> > yu_rcp = Teuchos::rcp( new std::vector<RealT> (pFEM->numU(), 0.0) );
      for (int i=0; i<pFEM->numU(); i++) {
      (*u_rcp)[i]  = (RealT)rand()/(RealT)RAND_MAX;
        (*yu_rcp)[i] = (RealT)rand()/(RealT)RAND_MAX;
      }
      ROL::StdVector<RealT> yu(yu_rcp);
      Teuchos::RCP<ROL::Vector<RealT> > yup = Teuchos::rcp(&yu,false);
      // Initialize Jacobian vector.
      Teuchos::RCP<std::vector<RealT> > jv_rcp  = Teuchos::rcp( new std::vector<RealT> (pFEM->numU(), 0.0) );
      ROL::StdVector<RealT> jv(jv_rcp);
      Teuchos::RCP<ROL::Vector<RealT> > jvp = Teuchos::rcp(&jv,false);
      // Initialize SimOpt Vectors 
      ROL::Vector_SimOpt<RealT> x(up,zp);
      ROL::Vector_SimOpt<RealT> y(yup,yzp);
      // Test equality constraint.
      pcon->checkApplyJacobian(x,y,jv,true);
      pcon->checkApplyAdjointJacobian(x,yu,true);
      pcon->checkApplyAdjointHessian(x,yu,y,true);
      // Test full objective function.
      pobj = Teuchos::rcp(new Objective_TopOpt<RealT>(pFEM,frac,reg));
      pobj->checkGradient(x,y,true);
      pobj->checkHessVec(x,y,true);
      // Test reduced objective function.
      robj = Teuchos::rcp(new ROL::Reduced_Objective_SimOpt<RealT>(pobj,pcon,up,pp));
      robj->checkGradient(z,yz,true);
      robj->checkHessVec(z,yz,true);
    }
    // Run optimization.
    for ( int j=0; j<npen; j++ ) {
      std::cout << "\nPenalty parameter: " << pen << "\n";
      for ( int i=0; i<nreg; i++ ) {
        std::cout << "\nMoreau-Yoshida regularization parameter: " << reg << "\n";
        // Initialize full objective function.
        pobj = Teuchos::rcp(new Objective_TopOpt<RealT>(pFEM,frac,reg,pen));
        // Initialize reduced objective function.
        robj = Teuchos::rcp(new ROL::Reduced_Objective_SimOpt<RealT>(pobj,pcon,up,pp));
        if ( !useTR ) {
          // Run line-search secant step.
          parlist->set("Descent Type","Quasi-Newton Method");
          parlist->set("Secant Type","Limited Memory SR1");
          maxit  = std::max(maxit-100,0);
          if ( maxit > 0 ) {
            gtol   = 1.e-4;
            stol   = 1.e-8;
            step   = Teuchos::rcp(new ROL::LineSearchStep<RealT>(*parlist));
            status = Teuchos::rcp(new ROL::StatusTest<RealT>(gtol,stol,maxit));
            algo   = Teuchos::rcp(new ROL::DefaultAlgorithm<RealT>(*step,*status,false));
            algo->run(z,*robj,bound,true);
          }
          // Run line-search Newton-Krylov step.
          parlist->set("Descent Type","Newton-Krylov");
          gtol   = 1.e-4;
          stol   = 1.e-8;
          step   = Teuchos::rcp(new ROL::LineSearchStep<RealT>(*parlist));
          status = Teuchos::rcp(new ROL::StatusTest<RealT>(gtol,stol,500));
          algo   = Teuchos::rcp(new ROL::DefaultAlgorithm<RealT>(*step,*status,false));
          algo->run(z,*robj,bound,true);
        }
        else {
          // Run trust-region step.
          step   = Teuchos::rcp(new ROL::TrustRegionStep<RealT>(*parlist));
          status = Teuchos::rcp(new ROL::StatusTest<RealT>(gtol,stol,maxit));
          algo   = Teuchos::rcp(new ROL::DefaultAlgorithm<RealT>(*step,*status,false));
          algo->run(z,*robj,bound,true);
        }
        // Compute volume.
        RealT vol = 0.0;
        for (int i=0; i<nx; i++) {
          for (int j=0; j<ny; j++) {
            vol += (*z_rcp)[i+j*nx];
          }
        }
        std::cout << "The volume fraction is " << vol/pFEM->numZ() << "\n";
        // Increase Moreau-Yoshida regularization parameter.
        reg *= 2.0;
      }
      reg   = 1.0;
      nreg += 5;
      pen  *= 10.0;
    }
    // Print to file.
    std::ofstream file;
    file.open("density.txt");
    RealT val = 0.0, vol = 0.0;
    for (int i=0; i<nx; i++) {
      for (int j=0; j<ny; j++) {
        val = (*z_rcp)[i+j*nx];
        file << i << "  " << j << "  " << val << "\n"; 
        vol += val;
      }
    }
    std::cout << "The volume fraction is " << vol/pFEM->numZ() << "\n";
    file.close();
  }
  catch (std::logic_error err) {
    *outStream << err.what() << "\n";
    errorFlag = -1000;
  }; // end try

  if (errorFlag != 0)
    std::cout << "End Result: TEST FAILED\n";
  else
    std::cout << "End Result: TEST PASSED\n";

  return 0;

}

