import React, { useState, useEffect, useRef, useCallback } from 'react';
import './App.css';
import TrajectoryViewer from './components/TrajectoryViewer';
import ControlPanel from './components/ControlPanel';

function App() {
  const [trajectory, setTrajectory] = useState(null);
  const [error, setError]           = useState(null);
  const [rtcStatus, setRtcStatus]   = useState('connecting');

  const dcRef = useRef(null);

  useEffect(() => {
    let pc        = new RTCPeerConnection();
    let cancelled = false;

    const dc = pc.createDataChannel('kalman', { ordered: true });
    dcRef.current = dc;

    dc.onopen  = () => { if (!cancelled) setRtcStatus('open'); };
    dc.onclose = () => { if (!cancelled) setRtcStatus('closed'); };
    dc.onerror = (e) => { if (!cancelled) setError('Data channel error: ' + (e.message || e)); };

    dc.onmessage = ({ data }) => {
      try {
        const msg = JSON.parse(data);
        if (msg.type === 'trajectory') setTrajectory(msg.points);
      } catch {}
    };

    (async () => {
      try {
        const offer = await pc.createOffer();
        await pc.setLocalDescription(offer);

        // Vanilla ICE: wait for all candidates to be gathered before sending the offer,
        // so the server can reply with a complete answer in one HTTP round-trip.
        await new Promise((resolve) => {
          if (pc.iceGatheringState === 'complete') { resolve(); return; }
          const handler = () => {
            if (pc.iceGatheringState === 'complete') {
              pc.removeEventListener('icegatheringstatechange', handler);
              resolve();
            }
          };
          pc.addEventListener('icegatheringstatechange', handler);
        });

        const resp = await fetch('/signal', {
          method:  'POST',
          headers: { 'Content-Type': 'application/json' },
          body:    JSON.stringify({
            type: pc.localDescription.type,
            sdp:  pc.localDescription.sdp,
          }),
        });

        if (!resp.ok) throw new Error(`Signaling failed (HTTP ${resp.status})`);

        const answer = await resp.json();
        await pc.setRemoteDescription(new RTCSessionDescription(answer));
      } catch (e) {
        if (!cancelled) {
          setError('WebRTC setup failed: ' + e.message);
          setRtcStatus('closed');
        }
      }
    })();

    return () => {
      cancelled = true;
      pc.close();
    };
  }, []);

  const handlePredict = useCallback((measurements, duration) => {
    const dc = dcRef.current;
    if (!dc || dc.readyState !== 'open') {
      setError('Not connected — wait for WebRTC to open');
      return;
    }
    setError(null);
    dc.send(JSON.stringify({ type: 'measurements', measurements, duration }));
  }, []);

  return (
    <div className="App" style={{ display: 'grid', gridTemplateColumns: '500px 1fr' }}>
      <ControlPanel
        onPredict={handlePredict}
        rtcStatus={rtcStatus}
        error={error}
      />
      <TrajectoryViewer trajectory={trajectory} />
    </div>
  );
}

export default App;
