import React, { useState } from 'react';
import './ControlPanel.css';

const STATUS_STYLE = {
  open:       { color: '#00ff88', label: '● Connected'    },
  connecting: { color: '#ffaa00', label: '● Connecting…'  },
  closed:     { color: '#ff4444', label: '● Disconnected' },
};

function ControlPanel({ onPredict, rtcStatus, error }) {
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
    const updated = [...measurements];
    updated[index][axis] = parseFloat(value) || 0;
    setMeasurements(updated);
  };

  const handlePredict = () => {
    if (measurements.length < 1) {
      alert('Please add at least one measurement');
      return;
    }
    onPredict(measurements, duration);
  };

  const { color, label } = STATUS_STYLE[rtcStatus] || STATUS_STYLE.closed;

  return (
    <div className="control-panel">
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
        <h2>3D Trajectory Prediction</h2>
        <span style={{
          padding: '3px 10px',
          borderRadius: '12px',
          background: color + '22',
          color,
          fontSize: '12px',
          fontWeight: 'bold',
          whiteSpace: 'nowrap',
        }}>
          {label}
        </span>
      </div>

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
        disabled={rtcStatus !== 'open'}
      >
        {rtcStatus === 'connecting' ? 'Connecting…' : 'Predict Trajectory'}
      </button>

      {error && <div className="error">{error}</div>}
    </div>
  );
}

export default ControlPanel;
