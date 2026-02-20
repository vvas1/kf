#pragma once
#include <Eigen/Dense>

struct KalmanFilter {
    Eigen::VectorXd x;
    Eigen::MatrixXd P;

    void predict(const Eigen::MatrixXd& F, const Eigen::MatrixXd& Q) {
        x = F * x;
        P = F * P * F.transpose() + Q;
    }

    void update(const Eigen::VectorXd& z,
                const Eigen::MatrixXd& H,
                const Eigen::MatrixXd& R) {
        Eigen::VectorXd y = z - H * x;
        Eigen::MatrixXd S = H * P * H.transpose() + R;
        Eigen::MatrixXd K = P * H.transpose() * S.inverse();
        x = x + K * y;
        int n = static_cast<int>(x.size());
        P = (Eigen::MatrixXd::Identity(n, n) - K * H) * P;
    }
};
