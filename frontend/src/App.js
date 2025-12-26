import React, { useState } from 'react';
import './App.css';
import TrajectoryViewer from './components/TrajectoryViewer';
import ControlPanel from './components/ControlPanel';

function App() {
  const [trajectory, setTrajectory] = useState(null);
  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState(null);

  const handlePredict = async (measurements, duration) => {
    setIsLoading(true);
    setError(null);
    
    try {
      const response = await fetch('http://localhost:8080/api/predict', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          duration: duration,
          measurements: measurements,
        }),
      });

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }

      const data = await response.json();
      setTrajectory(data.points);
    } catch (err) {
      setError(err.message);
      console.error('Error:', err);
    } finally {
      setIsLoading(false);
    }
  };

  return (
    <div className="App">
      <ControlPanel 
        onPredict={handlePredict} 
        isLoading={isLoading}
        error={error}
      />
      <TrajectoryViewer trajectory={trajectory} />
    </div>
  );
}

export default App;
