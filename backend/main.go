package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"

	"github.com/gorilla/mux"
)

type Point3D struct {
	X float64 `json:"x"`
	Y float64 `json:"y"`
	Z float64 `json:"z"`
}

type TrajectoryPrediction struct {
	Points []Point3D `json:"points"`
}

// Kalman Filter for 3D trajectory prediction
type KalmanFilter struct {
	// State vector [x, y, z, vx, vy, vz]
	State [6]float64
	// Covariance matrix
	Covariance [6][6]float64
	// Process noise
	Q [6][6]float64
	// Measurement noise
	R [3][3]float64
	// Measurement matrix
	H [3][6]float64
}

func NewKalmanFilter(initialPosition Point3D) *KalmanFilter {
	kf := &KalmanFilter{}

	// Initialize state with initial position and zero velocity
	kf.State[0] = initialPosition.X
	kf.State[1] = initialPosition.Y
	kf.State[2] = initialPosition.Z
	kf.State[3] = 0.0 // vx
	kf.State[4] = 0.0 // vy
	kf.State[5] = 0.0 // vz

	// Initialize covariance matrix with high uncertainty
	for i := 0; i < 6; i++ {
		for j := 0; j < 6; j++ {
			if i == j {
				kf.Covariance[i][j] = 1.0
			} else {
				kf.Covariance[i][j] = 0.0
			}
		}
	}

	// Initialize process noise covariance
	for i := 0; i < 6; i++ {
		kf.Q[i][i] = 0.01
	}

	// Initialize measurement noise covariance
	for i := 0; i < 3; i++ {
		kf.R[i][i] = 0.1
	}

	// Initialize measurement matrix H (observes position only)
	for i := 0; i < 3; i++ {
		kf.H[i][i] = 1.0
	}

	return kf
}

func (kf *KalmanFilter) Predict(dt float64) {
	// State transition matrix F
	var F [6][6]float64
	F[0][0] = 1.0 // x
	F[1][1] = 1.0 // y
	F[2][2] = 1.0 // z
	F[0][3] = dt  // x velocity
	F[1][4] = dt  // y velocity
	F[2][5] = dt  // z velocity
	F[3][3] = 1.0
	F[4][4] = 1.0
	F[5][5] = 1.0

	// Predict state: x' = F * x
	var newState [6]float64
	for i := 0; i < 6; i++ {
		newState[i] = 0.0
		for j := 0; j < 6; j++ {
			newState[i] += F[i][j] * kf.State[j]
		}
	}
	kf.State = newState

	// Predict covariance: P' = F * P * F' + Q
	var temp [6][6]float64
	// temp = F * P
	for i := 0; i < 6; i++ {
		for j := 0; j < 6; j++ {
			temp[i][j] = 0.0
			for k := 0; k < 6; k++ {
				temp[i][j] += F[i][k] * kf.Covariance[k][j]
			}
		}
	}
	// P = temp * F'
	for i := 0; i < 6; i++ {
		for j := 0; j < 6; j++ {
			kf.Covariance[i][j] = 0.0
			for k := 0; k < 6; k++ {
				kf.Covariance[i][j] += temp[i][k] * F[j][k]
			}
			kf.Covariance[i][j] += kf.Q[i][j]
		}
	}
}

func (kf *KalmanFilter) Update(measurement Point3D) {
	// Measurement z
	z := [3]float64{measurement.X, measurement.Y, measurement.Z}

	// Innovation: y = z - H * x
	var y [3]float64
	for i := 0; i < 3; i++ {
		sum := 0.0
		for j := 0; j < 6; j++ {
			sum += kf.H[i][j] * kf.State[j]
		}
		y[i] = z[i] - sum
	}

	// Innovation covariance: S = H * P * H' + R
	var S [3][3]float64
	// temp = H * P
	var temp [3][6]float64
	for i := 0; i < 3; i++ {
		for j := 0; j < 6; j++ {
			temp[i][j] = 0.0
			for k := 0; k < 6; k++ {
				temp[i][j] += kf.H[i][k] * kf.Covariance[k][j]
			}
		}
	}
	// S = temp * H' + R
	for i := 0; i < 3; i++ {
		for j := 0; j < 3; j++ {
			S[i][j] = 0.0
			for k := 0; k < 6; k++ {
				S[i][j] += temp[i][k] * kf.H[j][k]
			}
			S[i][j] += kf.R[i][j]
		}
	}

	// Kalman gain: K = P * H' * inv(S)
	var K [6][3]float64
	det := S[0][0]*(S[1][1]*S[2][2]-S[1][2]*S[2][1]) - S[0][1]*(S[1][0]*S[2][2]-S[1][2]*S[2][0]) + S[0][2]*(S[1][0]*S[2][1]-S[1][1]*S[2][0])
	invS := [3][3]float64{
		{(S[1][1]*S[2][2] - S[1][2]*S[2][1]) / det, -(S[0][1]*S[2][2] - S[0][2]*S[2][1]) / det, (S[0][1]*S[1][2] - S[0][2]*S[1][1]) / det},
		{-(S[1][0]*S[2][2] - S[1][2]*S[2][0]) / det, (S[0][0]*S[2][2] - S[0][2]*S[2][0]) / det, -(S[0][0]*S[1][2] - S[0][2]*S[1][0]) / det},
		{(S[1][0]*S[2][1] - S[1][1]*S[2][0]) / det, -(S[0][0]*S[2][1] - S[0][1]*S[2][0]) / det, (S[0][0]*S[1][1] - S[0][1]*S[1][0]) / det},
	}

	// K = P * H'
	var PHt [6][3]float64
	for i := 0; i < 6; i++ {
		for j := 0; j < 3; j++ {
			PHt[i][j] = 0.0
			for k := 0; k < 6; k++ {
				PHt[i][j] += kf.Covariance[i][k] * kf.H[j][k]
			}
		}
	}
	// K = PHt * inv(S)
	for i := 0; i < 6; i++ {
		for j := 0; j < 3; j++ {
			K[i][j] = 0.0
			for k := 0; k < 3; k++ {
				K[i][j] += PHt[i][k] * invS[k][j]
			}
		}
	}

	// Update state: x = x + K * y
	for i := 0; i < 6; i++ {
		for j := 0; j < 3; j++ {
			kf.State[i] += K[i][j] * y[j]
		}
	}

	// Update covariance: P = (I - K * H) * P
	var IKH [6][6]float64
	for i := 0; i < 6; i++ {
		IKH[i][i] = 1.0
		for j := 0; j < 3; j++ {
			for k := 0; k < 3; k++ {
				IKH[i][j] -= K[i][j] * kf.H[k][i]
			}
		}
	}

	var newCov [6][6]float64
	for i := 0; i < 6; i++ {
		for j := 0; j < 6; j++ {
			newCov[i][j] = 0.0
			for k := 0; k < 6; k++ {
				newCov[i][j] += IKH[i][k] * kf.Covariance[k][j]
			}
		}
	}
	kf.Covariance = newCov
}

func (kf *KalmanFilter) GetPosition() Point3D {
	return Point3D{
		X: kf.State[0],
		Y: kf.State[1],
		Z: kf.State[2],
	}
}

func predictTrajectory(duration float64, measurements []Point3D) []Point3D {
	if len(measurements) == 0 {
		return []Point3D{}
	}

	// Initialize Kalman filter with first measurement
	kf := NewKalmanFilter(measurements[0])

	// Update with all measurements
	for i := 1; i < len(measurements); i++ {
		dt := 0.5 // 0.5 seconds between measurements
		kf.Predict(dt)
		kf.Update(measurements[i])
	}

	// Predict future trajectory
	numPoints := int(duration / 0.5)
	trajectory := make([]Point3D, 0, numPoints)

	// Add current position
	trajectory = append(trajectory, kf.GetPosition())

	// Predict future points at 0.5 second intervals
	for i := 0; i < numPoints; i++ {
		kf.Predict(0.5)
		trajectory = append(trajectory, kf.GetPosition())
	}

	return trajectory
}

func handleTrajectoryPrediction(w http.ResponseWriter, r *http.Request) {
	var request struct {
		Duration     float64   `json:"duration"`
		Measurements []Point3D `json:"measurements"`
	}

	if err := json.NewDecoder(r.Body).Decode(&request); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	if len(request.Measurements) == 0 {
		http.Error(w, "Measurements array cannot be empty", http.StatusBadRequest)
		return
	}

	trajectory := predictTrajectory(request.Duration, request.Measurements)

	response := TrajectoryPrediction{
		Points: trajectory,
	}

	w.Header().Set("Content-Type", "application/json")
	w.Header().Set("Access-Control-Allow-Origin", "*")
	json.NewEncoder(w).Encode(response)
}

func handleHealth(w http.ResponseWriter, r *http.Request) {
	w.WriteHeader(http.StatusOK)
	w.Write([]byte("OK"))
}

func main() {
	r := mux.NewRouter()

	r.HandleFunc("/api/predict", handleTrajectoryPrediction).Methods("POST", "OPTIONS")
	r.HandleFunc("/health", handleHealth).Methods("GET")

	// CORS middleware
	r.Use(func(next http.Handler) http.Handler {
		return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			w.Header().Set("Access-Control-Allow-Origin", "*")
			w.Header().Set("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
			w.Header().Set("Access-Control-Allow-Headers", "Content-Type")

			if r.Method == "OPTIONS" {
				w.WriteHeader(http.StatusOK)
				return
			}

			next.ServeHTTP(w, r)
		})
	})

	port := "8080"
	fmt.Printf("Server starting on port %s\n", port)
	log.Fatal(http.ListenAndServe(":"+port, r))
}
