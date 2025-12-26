# 3D Trajectory Visualization Frontend

React frontend with Three.js for visualizing trajectory predictions.

## Installation

```bash
cd frontend
npm install
```

## Running

```bash
npm start
```

The app will start on `http://localhost:3000`

Make sure the backend is running on `http://localhost:8080`

## Features

- Input measurements (3D points)
- Set prediction duration
- Visualize predicted trajectory in 3D
- Interactive 3D viewer with orbit controls
  - Click and drag to rotate
  - Scroll to zoom
  - Right-click and drag to pan
- Red sphere: First measurement
- Cyan spheres: Predicted trajectory points (at 0.5 second intervals)
- Green line: Trajectory path
