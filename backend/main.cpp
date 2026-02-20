#include "httplib.h"
#include "json.hpp"
#include "kalman_filter.hpp"
#include "model_constant_accel.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>

using json = nlohmann::json;

static constexpr double DT          = 1.0;
static constexpr double MEASURE_VAR = 0.1;
static constexpr double INITIAL_VAR = 1.0;
static constexpr double PROCESS_VAR = 0.01;
static constexpr double PROCESS_VAR_ACCEL_FALLBACK = 0.5;

// Estimates acceleration process variance from position-only measurements via
// finite differences: a ≈ (p_{k+2} - 2*p_{k+1} + p_k) / dt^2
static double estimateProcessVarAccel(const std::vector<std::array<double, 3>>& pts, double dt) {
    if (pts.size() < 3 || dt <= 0.0)
        return PROCESS_VAR_ACCEL_FALLBACK;

    double dt2 = dt * dt;
    double sax = 0, say = 0, saz = 0;
    double sax2 = 0, say2 = 0, saz2 = 0;
    int n = 0;

    for (size_t i = 0; i + 2 < pts.size(); i++) {
        double ax = (pts[i+2][0] - 2*pts[i+1][0] + pts[i][0]) / dt2;
        double ay = (pts[i+2][1] - 2*pts[i+1][1] + pts[i][1]) / dt2;
        double az = (pts[i+2][2] - 2*pts[i+1][2] + pts[i][2]) / dt2;
        sax  += ax;  say  += ay;  saz  += az;
        sax2 += ax*ax; say2 += ay*ay; saz2 += az*az;
        n++;
    }

    double inv  = 1.0 / n;
    double varAx = std::max(0.0, sax2*inv - (sax*inv)*(sax*inv));
    double varAy = std::max(0.0, say2*inv - (say*inv)*(say*inv));
    double varAz = std::max(0.0, saz2*inv - (saz*inv)*(saz*inv));
    double est  = (varAx + varAy + varAz) / 3.0 * 10.0;
    return std::clamp(est, 0.01, 10.0);
}

static json predictTrajectory(double duration, const json& measurements) {
    std::vector<std::array<double, 3>> pts;
    pts.reserve(measurements.size());
    for (const auto& m : measurements)
        pts.push_back({m.at("x").get<double>(),
                       m.at("y").get<double>(),
                       m.at("z").get<double>()});

    ConstantAccel3D mdl;
    mdl.cfg = {INITIAL_VAR, PROCESS_VAR, estimateProcessVarAccel(pts, DT)};

    KalmanFilter kf;
    kf.x = mdl.initialState(pts[0][0], pts[0][1], pts[0][2]);
    kf.P = mdl.initialCovariance();

    auto H = mdl.H().cast<double>().eval();
    auto R = mdl.R(MEASURE_VAR).cast<double>().eval();

    for (size_t i = 1; i < pts.size(); i++) {
        kf.predict(mdl.F(DT), mdl.Q(DT));
        Eigen::Vector3d z(pts[i][0], pts[i][1], pts[i][2]);
        kf.update(z, H, R);
    }

    json result = json::array();
    result.push_back({{"x", kf.x(0)}, {"y", kf.x(1)}, {"z", kf.x(2)}});

    int numPoints = static_cast<int>(duration / DT);
    auto F = mdl.F(DT);
    for (int i = 0; i < numPoints; i++) {
        kf.x = F * kf.x;
        result.push_back({{"x", kf.x(0)}, {"y", kf.x(1)}, {"z", kf.x(2)}});
    }

    return result;
}

static void setCORSHeaders(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin",  "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

int main() {
    httplib::Server svr;

    svr.Options("/api/predict", [](const httplib::Request&, httplib::Response& res) {
        setCORSHeaders(res);
        res.status = 200;
    });

    svr.Post("/api/predict", [](const httplib::Request& req, httplib::Response& res) {
        setCORSHeaders(res);
        try {
            auto body = json::parse(req.body);
            const auto& meas = body.at("measurements");
            if (meas.empty()) {
                res.status = 400;
                res.set_content("Measurements array cannot be empty", "text/plain");
                return;
            }
            double duration = body.at("duration").get<double>();
            json response   = {{"points", predictTrajectory(duration, meas)}};
            res.set_content(response.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(std::string("Invalid request body: ") + e.what(), "text/plain");
        }
    });

    svr.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("OK", "text/plain");
    });

    const int port = 8080;
    std::cout << "Server starting on port " << port << "\n";
    if (!svr.listen("0.0.0.0", port)) {
        std::cerr << "Failed to bind to port " << port << "\n";
        return 1;
    }
    return 0;
}
