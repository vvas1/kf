import React, { useRef, useEffect } from 'react';
import './TrajectoryViewer.css';
import * as THREE from 'three';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls';

function TrajectoryViewer({ trajectory }) {
  const mountRef = useRef(null);
  const sceneRef = useRef(null);
  const rendererRef = useRef(null);
  const cameraRef = useRef(null);
  const controlsRef = useRef(null);
  const animationRef = useRef(null);

  useEffect(() => {
    if (!mountRef.current) return;

    // Create scene
    const scene = new THREE.Scene();
    scene.background = new THREE.Color(0x000000);
    sceneRef.current = scene;

    // Create camera
    const camera = new THREE.PerspectiveCamera(
      75,
      window.innerWidth / window.innerHeight,
      0.1,
      1000
    );
    camera.position.set(5, 5, 10);
    camera.lookAt(0, 0, 0);
    cameraRef.current = camera;

    // Create renderer
    const renderer = new THREE.WebGLRenderer({ antialias: true });
    renderer.setSize(window.innerWidth, window.innerHeight);
    renderer.shadowMap.enabled = true;
    mountRef.current.appendChild(renderer.domElement);
    rendererRef.current = renderer;

    // Add lighting
    const ambientLight = new THREE.AmbientLight(0xffffff, 0.6);
    scene.add(ambientLight);

    const directionalLight = new THREE.DirectionalLight(0xffffff, 0.8);
    directionalLight.position.set(10, 10, 5);
    directionalLight.castShadow = true;
    scene.add(directionalLight);

    // Add grid
    const gridHelper = new THREE.GridHelper(20, 20, 0x444444, 0x222222);
    scene.add(gridHelper);

    // Add axes helper
    const axesHelper = new THREE.AxesHelper(5);
    scene.add(axesHelper);

    // Add orbit controls
    const controls = new OrbitControls(camera, renderer.domElement);
    controls.enableDamping = true;
    controls.dampingFactor = 0.05;
    controlsRef.current = controls;

    // Handle window resize
    const handleResize = () => {
      camera.aspect = window.innerWidth / window.innerHeight;
      camera.updateProjectionMatrix();
      renderer.setSize(window.innerWidth, window.innerHeight);
    };
    window.addEventListener('resize', handleResize);

    // Animation loop
    const animate = () => {
      animationRef.current = requestAnimationFrame(animate);
      controls.update();
      renderer.render(scene, camera);
    };
    animate();

    return () => {
      window.removeEventListener('resize', handleResize);
      if (animationRef.current) {
        cancelAnimationFrame(animationRef.current);
      }
      if (mountRef.current && renderer.domElement) {
        mountRef.current.removeChild(renderer.domElement);
      }
      renderer.dispose();
    };
  }, []);

  useEffect(() => {
    if (!sceneRef.current || !trajectory || trajectory.length === 0) return;

    // Clear previous trajectory
    const objectsToRemove = [];
    sceneRef.current.children.forEach(child => {
      if (child.userData.isTrajectoryPoint || child.userData.isTrajectoryLine) {
        objectsToRemove.push(child);
      }
    });
    objectsToRemove.forEach(obj => sceneRef.current.remove(obj));

    // Create trajectory points
    const points = trajectory.map(p => new THREE.Vector3(p.x, p.y, p.z));
    
    // Calculate bounding box
    const box = new THREE.Box3().setFromPoints(points);
    const center = box.getCenter(new THREE.Vector3());
    const size = box.getSize(new THREE.Vector3());

    // Create line geometry
    const geometry = new THREE.BufferGeometry().setFromPoints(points);
    const material = new THREE.LineBasicMaterial({
      color: 0x00ff00,
      linewidth: 3
    });
    const line = new THREE.Line(geometry, material);
    line.userData.isTrajectoryLine = true;
    sceneRef.current.add(line);

    // Add spheres for each point
    points.forEach((point, index) => {
      const sphereGeometry = new THREE.SphereGeometry(0.1, 16, 16);
      const sphereMaterial = new THREE.MeshBasicMaterial({ color: index === 0 ? 0xff0000 : 0x00ffff });
      const sphere = new THREE.Mesh(sphereGeometry, sphereMaterial);
      sphere.position.copy(point);
      sphere.userData.isTrajectoryPoint = true;
      sceneRef.current.add(sphere);
    });

    // Center camera on trajectory
    if (cameraRef.current && controlsRef.current) {
      const maxDim = Math.max(size.x, size.y, size.z) || 10;
      const fov = cameraRef.current.fov * (Math.PI / 180);
      let cameraZ = maxDim / (2 * Math.tan(fov / 2));
      cameraZ *= 2.5; // Add padding

      const newPosition = new THREE.Vector3(
        center.x + cameraZ / 3,
        center.y + cameraZ / 3,
        center.z + cameraZ
      );
      
      cameraRef.current.position.copy(newPosition);
      controlsRef.current.target.copy(center);
      controlsRef.current.update();
    }
  }, [trajectory]);

  return <div ref={mountRef} className="trajectory-viewer" />;
}

export default TrajectoryViewer;
