/**
 * @file MPCExample.cpp
 * @author Giulio Romualdi
 * @copyright Released under the terms of the LGPLv2.1 or later, see LGPL.TXT
 * @date 2018
 */


// osqp-eigen
#include "OsqpEigen/OsqpEigen.h"

// eigen
#include <Eigen/Dense>

#include <iostream>

#define nx 12
#define nu 4

void setDynamicsMatrices(Eigen::Matrix<double, nx, nx> &a, Eigen::Matrix<double, nx, nu> &b)
{
    a << 1.,      0.,     0., 0., 0., 0., 0.1,     0.,     0.,  0.,     0.,     0.    ,
        0.,      1.,     0., 0., 0., 0., 0.,      0.1,    0.,  0.,     0.,     0.    ,
        0.,      0.,     1., 0., 0., 0., 0.,      0.,     0.1, 0.,     0.,     0.    ,
        0.0488,  0.,     0., 1., 0., 0., 0.0016,  0.,     0.,  0.0992, 0.,     0.    ,
        0.,     -0.0488, 0., 0., 1., 0., 0.,     -0.0016, 0.,  0.,     0.0992, 0.    ,
        0.,      0.,     0., 0., 0., 1., 0.,      0.,     0.,  0.,     0.,     0.0992,
        0.,      0.,     0., 0., 0., 0., 1.,      0.,     0.,  0.,     0.,     0.    ,
        0.,      0.,     0., 0., 0., 0., 0.,      1.,     0.,  0.,     0.,     0.    ,
        0.,      0.,     0., 0., 0., 0., 0.,      0.,     1.,  0.,     0.,     0.    ,
        0.9734,  0.,     0., 0., 0., 0., 0.0488,  0.,     0.,  0.9846, 0.,     0.    ,
        0.,     -0.9734, 0., 0., 0., 0., 0.,     -0.0488, 0.,  0.,     0.9846, 0.    ,
        0.,      0.,     0., 0., 0., 0., 0.,      0.,     0.,  0.,     0.,     0.9846;

    b << 0.,      -0.0726,  0.,     0.0726,
        -0.0726,  0.,      0.0726, 0.    ,
        -0.0152,  0.0152, -0.0152, 0.0152,
        -0.,     -0.0006, -0.,     0.0006,
        0.0006,   0.,     -0.0006, 0.0000,
        0.0106,   0.0106,  0.0106, 0.0106,
        0,       -1.4512,  0.,     1.4512,
        -1.4512,  0.,      1.4512, 0.    ,
        -0.3049,  0.3049, -0.3049, 0.3049,
        -0.,     -0.0236,  0.,     0.0236,
        0.0236,   0.,     -0.0236, 0.    ,
        0.2107,   0.2107,  0.2107, 0.2107;
}


void setInequalityConstraints(Eigen::Matrix<double, nx, 1> &xMax, Eigen::Matrix<double, nx, 1> &xMin,
                              Eigen::Matrix<double, nu, 1> &uMax, Eigen::Matrix<double, nu, 1> &uMin)
{
    double u0 = 10.5916;

    // input inequality constraints
    uMin << 9.6 - u0,
        9.6 - u0,
        9.6 - u0,
        9.6 - u0;

    uMax << 13 - u0,
        13 - u0,
        13 - u0,
        13 - u0;

    // state inequality constraints
    xMin << -M_PI/6,-M_PI/6,-OsqpEigen::INFTY,-OsqpEigen::INFTY,-OsqpEigen::INFTY,-1.,
        -OsqpEigen::INFTY, -OsqpEigen::INFTY,-OsqpEigen::INFTY,-OsqpEigen::INFTY,
        -OsqpEigen::INFTY,-OsqpEigen::INFTY;

    xMax << M_PI/6,M_PI/6, OsqpEigen::INFTY,OsqpEigen::INFTY,OsqpEigen::INFTY,
        OsqpEigen::INFTY, OsqpEigen::INFTY,OsqpEigen::INFTY,OsqpEigen::INFTY,
        OsqpEigen::INFTY,OsqpEigen::INFTY,OsqpEigen::INFTY;
}

void setWeightMatrices(Eigen::DiagonalMatrix<double, nx> &Q, Eigen::DiagonalMatrix<double, nu> &R)
{
    Q.diagonal() << 0, 0, 10., 10., 10., 10., 0, 0, 0, 5., 5., 5.;
    R.diagonal() << 0.1, 0.1, 0.1, 0.1;
}

void castMPCToQPHessian(const Eigen::DiagonalMatrix<double, nx> &Q, const Eigen::DiagonalMatrix<double, nu> &R, int mpcWindow,
                        Eigen::SparseMatrix<double> &hessianMatrix)
{

    hessianMatrix.resize(nx*(mpcWindow+1) + nu * mpcWindow, nx*(mpcWindow+1) + nu * mpcWindow);

    //populate hessian matrix
    for(int i = 0; i<nx*(mpcWindow+1) + nu * mpcWindow; i++){
        if(i < nx*(mpcWindow+1)){
            int posQ=i%nx;
            float value = Q.diagonal()[posQ];
            if(value != 0)
                hessianMatrix.insert(i,i) = value;
        }
        else{
            int posR=i%nu;
            float value = R.diagonal()[posR];
            if(value != 0)
                hessianMatrix.insert(i,i) = value;
        }
    }
}

void castMPCToQPGradient(const Eigen::DiagonalMatrix<double, nx> &Q, const Eigen::Matrix<double, nx, 1> &xRef, int mpcWindow,
                         Eigen::VectorXd &gradient)
{

    Eigen::Matrix<double,nx,1> Qx_ref;
    Qx_ref = Q * (-xRef);

    // populate the gradient vector
    gradient = Eigen::VectorXd::Zero(nx*(mpcWindow+1) +  nu*mpcWindow, 1);
    for(int i = 0; i<nx*(mpcWindow+1); i++){
        int posQ=i%nx;
        float value = Qx_ref(posQ,0);
        gradient(i,0) = value;
    }
}

void castMPCToQPConstraintMatrix(const Eigen::Matrix<double, nx, nx> &dynamicMatrix, const Eigen::Matrix<double, nx, nu> &controlMatrix,
                                 int mpcWindow, Eigen::SparseMatrix<double> &constraintMatrix)
{
    constraintMatrix.resize(nx*(mpcWindow+1)  + nx*(mpcWindow+1) + nu * mpcWindow, nx*(mpcWindow+1) + nu * mpcWindow);

    // populate linear constraint matrix
    for(int i = 0; i<nx*(mpcWindow+1); i++){
        constraintMatrix.insert(i,i) = -1;
    }

    for(int i = 0; i < mpcWindow; i++)
        for(int j = 0; j<nx; j++)
            for(int k = 0; k<nx; k++){
                float value = dynamicMatrix(j,k);
                if(value != 0){
                    constraintMatrix.insert(nx * (i+1) + j, nx * i + k) = value;
                }
            }

    for(int i = 0; i < mpcWindow; i++)
        for(int j = 0; j < nx; j++)
            for(int k = 0; k < nu; k++){
                float value = controlMatrix(j,k);
                if(value != 0){
                    constraintMatrix.insert(nx*(i+1)+j, nu*i+k+nx*(mpcWindow + 1)) = value;
                }
            }

    for(int i = 0; i<nx*(mpcWindow+1) + nu*mpcWindow; i++){
        constraintMatrix.insert(i+(mpcWindow+1)*nx,i) = 1;
    }
}

void castMPCToQPConstraintVectors(const Eigen::Matrix<double, nx, 1> &xMax, const Eigen::Matrix<double, nx, 1> &xMin,
                                   const Eigen::Matrix<double, nu, 1> &uMax, const Eigen::Matrix<double, nu, 1> &uMin,
                                   const Eigen::Matrix<double, nx, 1> &x0,
                                   int mpcWindow, Eigen::VectorXd &lowerBound, Eigen::VectorXd &upperBound)
{
    // evaluate the lower and the upper inequality vectors
    Eigen::VectorXd lowerInequality = Eigen::MatrixXd::Zero(nx*(mpcWindow+1) +  nu * mpcWindow, 1);
    Eigen::VectorXd upperInequality = Eigen::MatrixXd::Zero(nx*(mpcWindow+1) +  nu * mpcWindow, 1);
    for(int i=0; i<mpcWindow+1; i++){
        lowerInequality.block(nx*i,0,nx,1) = xMin;
        upperInequality.block(nx*i,0,nx,1) = xMax;
    }
    for(int i=0; i<mpcWindow; i++){
        lowerInequality.block(nu * i + nx * (mpcWindow + 1), 0, nu, 1) = uMin;
        upperInequality.block(nu * i + nx * (mpcWindow + 1), 0, nu, 1) = uMax;
    }

    // evaluate the lower and the upper equality vectors
    Eigen::VectorXd lowerEquality = Eigen::MatrixXd::Zero(nx*(mpcWindow+1),1 );
    Eigen::VectorXd upperEquality;
    lowerEquality.block(0,0,nx,1) = -x0;
    upperEquality = lowerEquality;
    lowerEquality = lowerEquality;

    // merge inequality and equality vectors
    lowerBound = Eigen::MatrixXd::Zero(2*nx*(mpcWindow+1) +  nu*mpcWindow,1 );
    lowerBound << lowerEquality,
        lowerInequality;

    upperBound = Eigen::MatrixXd::Zero(2*nx*(mpcWindow+1) +  nu*mpcWindow,1 );
    upperBound << upperEquality,
        upperInequality;
}


void updateConstraintVectors(const Eigen::Matrix<double, nx, 1> &x0,
                             Eigen::VectorXd &lowerBound, Eigen::VectorXd &upperBound)
{
    lowerBound.block(0,0,nx,1) = -x0;
    upperBound.block(0,0,nx,1) = -x0;
}


double getErrorNorm(const Eigen::Matrix<double, nx, 1> &x,
                    const Eigen::Matrix<double, nx, 1> &xRef)
{
    // evaluate the error
    Eigen::Matrix<double, nx, 1> error = x - xRef;

    // return the norm
    return error.norm();
}


int main()
{
    // set the preview window
    int mpcWindow = 20;

    // allocate the dynamics matrices
    Eigen::Matrix<double, nx, nx> a;
    Eigen::Matrix<double, nx, nu> b;

    // allocate the constraints vector
    Eigen::Matrix<double, nx, 1> xMax;
    Eigen::Matrix<double, nx, 1> xMin;
    Eigen::Matrix<double, nu, 1> uMax;
    Eigen::Matrix<double, nu, 1> uMin;

    // allocate the weight matrices
    Eigen::DiagonalMatrix<double, nx> Q;
    Eigen::DiagonalMatrix<double, nu> R;

    // allocate the initial and the reference state space
    Eigen::Matrix<double, nx, 1> x0;
    Eigen::Matrix<double, nx, 1> xRef;

    // allocate QP problem matrices and vectores
    Eigen::SparseMatrix<double> hessian;
    Eigen::VectorXd gradient;
    Eigen::SparseMatrix<double> linearMatrix;
    Eigen::VectorXd lowerBound;
    Eigen::VectorXd upperBound;

    // set the initial and the desired states
    x0 << 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ;
    xRef <<  0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0;

    // set MPC problem quantities
    setDynamicsMatrices(a, b);
    setInequalityConstraints(xMax, xMin, uMax, uMin);
    setWeightMatrices(Q, R);

    // cast the MPC problem as QP problem
    castMPCToQPHessian(Q, R, mpcWindow, hessian);
    castMPCToQPGradient(Q, xRef, mpcWindow, gradient);
    castMPCToQPConstraintMatrix(a, b, mpcWindow, linearMatrix);
    castMPCToQPConstraintVectors(xMax, xMin, uMax, uMin, x0, mpcWindow, lowerBound, upperBound);

    // instantiate the solver
    OsqpEigen::Solver solver;

    // settings
    //solver.settings()->setVerbosity(false);
    solver.settings()->setWarmStart(true);

    // set the initial data of the QP solver
    solver.data()->setNumberOfVariables(nx * (mpcWindow + 1) + nu * mpcWindow);
    solver.data()->setNumberOfConstraints(2 * nx * (mpcWindow + 1) +  nu * mpcWindow);
    if(!solver.data()->setHessianMatrix(hessian)) return 1;
    if(!solver.data()->setGradient(gradient)) return 1;
    if(!solver.data()->setLinearConstraintsMatrix(linearMatrix)) return 1;
    if(!solver.data()->setLowerBound(lowerBound)) return 1;
    if(!solver.data()->setUpperBound(upperBound)) return 1;

    // instantiate the solver
    if(!solver.initSolver()) return 1;

    // controller input and QPSolution vector
    Eigen::Vector4d ctr;
    Eigen::VectorXd QPSolution;

    // number of iteration steps
    int numberOfSteps = 50;

    for (int i = 0; i < numberOfSteps; i++){

        // solve the QP problem
        if(!solver.solve()) return 1;

        // get the controller input
        QPSolution = solver.getSolution();
        ctr = QPSolution.block(nx * (mpcWindow + 1), 0, nu, 1);

        // save data into file
        auto x0Data = x0.data();

        // propagate the model
        x0 = a * x0 + b * ctr;

        // update the constraint bound
        updateConstraintVectors(x0, lowerBound, upperBound);
        if(!solver.updateBounds(lowerBound, upperBound)) return 1;
      }
    return 0;
}
