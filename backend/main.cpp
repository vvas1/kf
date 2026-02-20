#include "httplib.h"
#include "json.hpp"
#include "kalman_filter.hpp"
#include "model_constant_accel.hpp"
#include <rtc/rtc.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <vector>

using json = nlohmann::json;

static constexpr double DT                         = 1.0;
static constexpr double MEASURE_VAR                = 0.1;
static constexpr double INITIAL_VAR                = 1.0;
static constexpr double PROCESS_VAR                = 0.01;
static constexpr double PROCESS_VAR_ACCEL_FALLBACK = 0.5;

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

// ── per-connection state ──────────────────────────────────────────────────────

struct PeerSession {
    std::shared_ptr<rtc::PeerConnection> pc;
    std::shared_ptr<rtc::DataChannel>    dc;
};

static std::map<std::string, std::shared_ptr<PeerSession>> gSessions;
static std::mutex                                           gMutex;

static std::string genId() {
    static std::mt19937 rng(std::random_device{}());
    static std::uniform_int_distribution<int> dist(0, 15);
    std::string id(16, '0');
    for (auto& c : id) c = "0123456789abcdef"[dist(rng)];
    return id;
}

static void setCORSHeaders(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin",  "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

int main() {
    // Suppress libdatachannel verbose logs
    rtc::InitLogger(rtc::LogLevel::Warning);

    httplib::Server svr;

    svr.Options("/signal", [](const httplib::Request&, httplib::Response& res) {
        setCORSHeaders(res);
        res.status = 200;
    });

    svr.Post("/signal", [](const httplib::Request& req, httplib::Response& res) {
        setCORSHeaders(res);
        try {
            auto body     = json::parse(req.body);
            auto offerSdp = body.at("sdp").get<std::string>();

            rtc::Configuration cfg;
            cfg.portRangeBegin = 10000;
            cfg.portRangeEnd   = 10010;

            // Optional STUN server via env var (useful when backend is behind Docker NAT)
            const char* stun = std::getenv("STUN_SERVER");
            if (stun && *stun)
                cfg.iceServers.emplace_back(std::string(stun));

            auto pc      = std::make_shared<rtc::PeerConnection>(cfg);
            auto session = std::make_shared<PeerSession>();
            session->pc  = pc;

            // Wait for full ICE gathering before returning answer (vanilla ICE)
            auto gatherDone = std::make_shared<std::promise<std::string>>();
            pc->onGatheringStateChange([pc, gatherDone](rtc::PeerConnection::GatheringState st) mutable {
                if (st == rtc::PeerConnection::GatheringState::Complete)
                    if (auto d = pc->localDescription())
                        try { gatherDone->set_value(std::string(*d)); } catch (...) {}
            });

            pc->onDataChannel([session](std::shared_ptr<rtc::DataChannel> dc) mutable {
                session->dc = dc;
                dc->onMessage([dc](rtc::message_variant data) {
                    auto* s = std::get_if<std::string>(&data);
                    if (!s) return;
                    try {
                        auto msg  = json::parse(*s);
                        auto type = msg.at("type").get<std::string>();
                        if (type == "measurements" && dc->isOpen()) {
                            auto pts = predictTrajectory(
                                msg.value("duration", 10.0),
                                msg.at("measurements"));
                            json resp = {{"type", "trajectory"}, {"points", pts}};
                            dc->send(resp.dump());
                        }
                    } catch (...) {}
                });
            });

            std::string id = genId();
            pc->onStateChange([id](rtc::PeerConnection::State st) {
                if (st == rtc::PeerConnection::State::Closed    ||
                    st == rtc::PeerConnection::State::Failed     ||
                    st == rtc::PeerConnection::State::Disconnected) {
                    std::lock_guard<std::mutex> lk(gMutex);
                    gSessions.erase(id);
                }
            });

            pc->setRemoteDescription(rtc::Description(offerSdp, rtc::Description::Type::Offer));
            pc->setLocalDescription();

            auto fut = gatherDone->get_future();
            if (fut.wait_for(std::chrono::seconds(10)) != std::future_status::ready) {
                res.status = 504;
                res.set_content("ICE gathering timeout", "text/plain");
                return;
            }

            {
                std::lock_guard<std::mutex> lk(gMutex);
                gSessions[id] = session;
            }

            json answer = {{"type", "answer"}, {"sdp", fut.get()}};
            res.set_content(answer.dump(), "application/json");

        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(std::string("Signal error: ") + e.what(), "text/plain");
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
