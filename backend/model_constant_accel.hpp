#pragma once
#include <Eigen/Dense>

// 3D constant-acceleration Kalman model.
// State vector: [x, y, z, vx, vy, vz, ax, ay, az]
// Dynamics: x' = x + vx*dt + 0.5*ax*dt^2, vx' = vx + ax*dt, ax' = ax (same for y, z)

struct ConstantAccel3DConfig {
    double initialVariance;
    double processVariance;
    double processVarianceAccel;
};

struct ConstantAccel3D {
    static constexpr int STATE_DIM = 9;
    static constexpr int OBS_DIM   = 3;

    ConstantAccel3DConfig cfg;

    Eigen::Matrix<double, STATE_DIM, STATE_DIM> F(double dt) const {
        auto mat = Eigen::Matrix<double, STATE_DIM, STATE_DIM>::Identity();
        double dt2 = 0.5 * dt * dt;
        for (int i = 0; i < 3; i++) {
            mat(i,   i+3) = dt;
            mat(i,   i+6) = dt2;
            mat(i+3, i+6) = dt;
        }
        return mat;
    }

    Eigen::Matrix<double, STATE_DIM, STATE_DIM> Q(double dt) const {
        auto mat = Eigen::Matrix<double, STATE_DIM, STATE_DIM>::Zero();
        for (int i = 0; i < 6; i++)
            mat(i, i) = dt * cfg.processVariance;
        for (int i = 6; i < STATE_DIM; i++)
            mat(i, i) = dt * cfg.processVarianceAccel;
        return mat;
    }

    // Observation model: observe position only (first 3 components of state)
    Eigen::Matrix<double, OBS_DIM, STATE_DIM> H() const {
        auto mat = Eigen::Matrix<double, OBS_DIM, STATE_DIM>::Zero();
        mat(0, 0) = mat(1, 1) = mat(2, 2) = 1.0;
        return mat;
    }

    Eigen::Matrix<double, OBS_DIM, OBS_DIM> R(double variance) const {
        return Eigen::Matrix<double, OBS_DIM, OBS_DIM>::Identity() * variance;
    }

    Eigen::Matrix<double, STATE_DIM, 1> initialState(double x, double y, double z) const {
        Eigen::Matrix<double, STATE_DIM, 1> s = Eigen::Matrix<double, STATE_DIM, 1>::Zero();
        s(0) = x; s(1) = y; s(2) = z;
        return s;
    }

    Eigen::Matrix<double, STATE_DIM, STATE_DIM> initialCovariance() const {
        return Eigen::Matrix<double, STATE_DIM, STATE_DIM>::Identity() * cfg.initialVariance;
    }
};
