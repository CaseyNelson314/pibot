// App.tsx
import React, { useState, useEffect, useRef } from "react";

const WS_URL = "ws://pibot.local:9000";

interface Wheel {
  x: number;
  y: number;
  turn: number;
}

interface Arm {
  axis1: number;
  axis2: number;
  axis3: number;
  axis4: number;
  axis5: number;
}

interface RobotState {
  wheel: Wheel;
  arm: Arm;
}

const App: React.FC = () => {
  const ws = useRef<WebSocket | null>(null);

  const [wheel, setWheel] = useState<Wheel>({ x: 0, y: 0, turn: 0 });
  const [arm, setArm] = useState<Arm>({ axis1: 0, axis2: 0, axis3: 0, axis4: 0, axis5: 0 });

  // 送信中のJSONを表示するためのステート
  const [sentJson, setSentJson] = useState<string>("{}");

  // WebSocket接続
  useEffect(() => {
    ws.current = new WebSocket(WS_URL);

    ws.current.onopen = () => console.log("WebSocket connected");
    ws.current.onclose = () => console.log("WebSocket disconnected");
    ws.current.onerror = (err) => console.error("WebSocket error:", err);

    return () => ws.current?.close();
  }, []);

  // JSON送信
  const sendData = () => {
    const data: RobotState = { wheel, arm };
    const json = JSON.stringify(data, null, 2); // 見やすく整形
    setSentJson(json);

    if (ws.current?.readyState === WebSocket.OPEN) {
      ws.current.send(json);
    }
  };

  // wheelやarmが変わるたび送信
  useEffect(() => {
    sendData();
  }, [wheel, arm]);

  // スライダー生成
  const createSlider = (
    label: string,
    value: number,
    min: number,
    max: number,
    step: number,
    onChange: (val: number) => void
  ) => (
    <div style={{ marginBottom: "10px" }}>
      <label>{label}: {value.toFixed(2)}</label>
      <input
        type="range"
        min={min}
        max={max}
        step={step}
        value={value}
        onChange={(e) => onChange(parseFloat(e.target.value))}
        style={{ width: "100%" }}
      />
    </div>
  );

  // 横並びのスタイル
  const containerStyle: React.CSSProperties = {
    display: "flex",
    justifyContent: "space-between",
    gap: "20px"
  };

  const columnStyle: React.CSSProperties = {
    flex: 1,
    minWidth: "200px"
  };

  return (
    <div style={{ padding: "20px" }}>
      <div style={containerStyle}>
        {/* Wheel */}
        <div style={columnStyle}>
          <h2>Wheel Control</h2>
          {createSlider("X", wheel.x, -1, 1, 0.01, (val) => setWheel({ ...wheel, x: val }))}
          {createSlider("Y", wheel.y, -1, 1, 0.01, (val) => setWheel({ ...wheel, y: val }))}
          {createSlider("Turn", wheel.turn, -1, 1, 0.01, (val) => setWheel({ ...wheel, turn: val }))}
        </div>

        {/* Arm */}
        <div style={columnStyle}>
          <h2>Arm Control</h2>
          {createSlider("Axis1", arm.axis1, 0, Math.PI * (3 / 2), 0.01, (val) => setArm({ ...arm, axis1: val }))}
          {createSlider("Axis2", arm.axis2, 0, Math.PI * (3 / 2), 0.01, (val) => setArm({ ...arm, axis2: val }))}
          {createSlider("Axis3", arm.axis3, 0, Math.PI * (3 / 2), 0.01, (val) => setArm({ ...arm, axis3: val }))}
          {createSlider("Axis4", arm.axis4, 0, Math.PI * (3 / 2), 0.01, (val) => setArm({ ...arm, axis4: val }))}
          {createSlider("Axis5", arm.axis5, 0, Math.PI * (3 / 2), 0.01, (val) => setArm({ ...arm, axis5: val }))}
        </div>

        {/* JSON表示 */}
        <div style={columnStyle}>
          <h2>Sent JSON</h2>
          <pre style={{ background: "#f0f0f0", padding: "10px", borderRadius: "5px", overflowX: "auto" }}>
            {sentJson}
          </pre>
        </div>
      </div>
    </div>
  );
};

export default App;