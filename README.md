# 3D Trajectory Prediction Application

A full-stack application for predicting and visualizing 3D trajectories using Kalman filtering.

## Architecture

- **Backend**: Go service with Kalman filter implementation
- **Frontend**: React app with Three.js for 3D visualization

## Getting Started

### Prerequisites

- Go 1.21 or higher
- Node.js 16+ and npm

### Step 1: Start the Backend

```bash
cd backend
go mod tidy
go run main.go
```

Backend runs on `http://localhost:8080`

### Step 2: Start the Frontend

In a new terminal:

```bash
cd frontend
npm install
npm start
```

Frontend runs on `http://localhost:3000` and will automatically open in your browser.

### Usage

1. The frontend has a control panel on the left
2. Add or modify 3D measurement points
3. Set the prediction duration (in seconds)
4. Click "Predict Trajectory"
5. View the predicted trajectory in the 3D viewer
6. Use mouse to rotate, zoom, and pan the 3D view

## How It Works

1. **Input**: Provide 3D position measurements
2. **Kalman Filter**: Backend uses Kalman filter to estimate velocity and predict future trajectory
3. **Output**: Returns trajectory points at 0.5 second intervals
4. **Visualization**: Frontend renders the trajectory in a 3D space using Three.js

## Features

- 3D trajectory prediction using Kalman filtering
- Points generated at 0.5 second intervals
- Interactive 3D visualization
- Adjustable prediction duration
- Multiple input measurements support

## API

### POST /api/predict

**Request:**
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
