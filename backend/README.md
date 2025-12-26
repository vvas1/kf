# Trajectory Prediction Backend

Go backend service implementing Kalman filter for 3D trajectory prediction.

## How to Run

```bash
cd backend
go mod tidy
go run main.go
```

The server will start on `http://localhost:8080`

## API Endpoints

### POST /api/predict

Predicts a 3D trajectory using Kalman filtering.

**Request Body:**
```json
{
  "duration": 10.0,
  "measurements": [
    { "x": 0.0, "y": 0.0, "z": 0.0 },
    { "x": 1.0, "y": 0.5, "z": 0.2 },
    { "x": 2.0, "y": 1.0, "z": 0.4 }
  ]
}
```

**Response:**
```json
{
  "points": [
    { "x": 2.0, "y": 1.0, "z": 0.4 },
    { "x": 3.0, "y": 1.5, "z": 0.6 },
    ...
  ]
}
```

Points are spaced at 0.5 second intervals.

### GET /health

Health check endpoint.
