import React, { useState } from 'react';
import './ControlPanel.css';

function ControlPanel({ onPredict, isLoading, error }) {
  const [duration, setDuration] = useState(10);
  const [measurements, setMeasurements] = useState([
    { x: 0, y: 0, z: 0 },
    { x: 1, y: 0.5, z: 0.8 },
    { x: 2, y: 1.2, z: 1.2 },
    { x: 3, y: 2.0, z: 1.0 },
    { x: 4, y: 2.5, z: 0.5 },
    { x: 5, y: 3.2, z: 0.2 },
  ]);

  const addMeasurement = () => {
    setMeasurements([...measurements, { x: 0, y: 0, z: 0 }]);
  };

  const removeMeasurement = (index) => {
    setMeasurements(measurements.filter((_, i) => i !== index));
  };

  const updateMeasurement = (index, axis, value) => {
    const newMeasurements = [...measurements];
    newMeasurements[index][axis] = parseFloat(value) || 0;
    setMeasurements(newMeasurements);
  };

  const handlePredict = () => {
    if (measurements.length < 1) {
      alert('Please add at least one measurement');
      return;
    }
    onPredict(measurements, duration);
  };

  return (
    <div className="control-panel">
      <h2>3D Trajectory Prediction</h2>

      <div className="input-group">
        <label>Prediction Duration (seconds):</label>
        <input
          type="number"
          min="1"
          max="60"
          value={duration}
          onChange={(e) => setDuration(parseFloat(e.target.value) || 1)}
        />
      </div>

      <div className="measurements-section">
        <div className="section-header">
          <h3>Measurements</h3>
          <button onClick={addMeasurement} className="add-btn">+ Add</button>
        </div>

        <div className="measurements-list">
          {measurements.map((measurement, index) => (
            <div key={index} className="measurement-item">
              <span className="measurement-label">Point {index + 1}:</span>
              <div className="coords">
                <input
                  type="number"
                  step="0.1"
                  placeholder="X"
                  value={measurement.x}
                  onChange={(e) => updateMeasurement(index, 'x', e.target.value)}
                />
                <input
                  type="number"
                  step="0.1"
                  placeholder="Y"
                  value={measurement.y}
                  onChange={(e) => updateMeasurement(index, 'y', e.target.value)}
                />
                <input
                  type="number"
                  step="0.1"
                  placeholder="Z"
                  value={measurement.z}
                  onChange={(e) => updateMeasurement(index, 'z', e.target.value)}
                />
              </div>
              {measurements.length > 1 && (
                <button
                  onClick={() => removeMeasurement(index)}
                  className="remove-btn"
                >
                  Remove
                </button>
              )}
            </div>
          ))}
        </div>
      </div>

      <button
        onClick={handlePredict}
        className="predict-btn"
        disabled={isLoading}
      >
        {isLoading ? 'Predicting...' : 'Predict Trajectory'}
      </button>

      {error && <div className="error">{error}</div>}
    </div>
  );
}

export default ControlPanel;
