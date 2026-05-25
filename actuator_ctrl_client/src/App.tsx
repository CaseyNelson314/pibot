// App.tsx
import React, { useState, useEffect, useRef, useCallback } from "react";

const DEFAULT_WS_URL = "ws://pibot.local:9000";
const SEND_INTERVAL_MS = 50; // 20Hz。ステート変化頻度と送信頻度を分離してWSの溢れを防ぐ

interface Wheel {
  x: number;
  y: number;
  turn: number;
}

interface Servo {
  camera_left_right: number;
  camera_up_down: number;
}

interface RobotState {
  wheel: Wheel;
  servo: Servo;
}

type ConnStatus = "disconnected" | "connecting" | "connected" | "error";

const STATUS_META: Record<ConnStatus, { label: string; color: string }> = {
  disconnected: { label: "DISCONNECTED", color: "#6b7280" },
  connecting: { label: "CONNECTING", color: "#eab308" },
  connected: { label: "CONNECTED", color: "#22c55e" },
  error: { label: "ERROR", color: "#ef4444" },
};

// ───────────────────────────────────────────────────────────
// Joystick: pointer events でマウス/タッチ統一。中心からのオフセットを
// -1..1 に正規化して onChange。離したら (0,0) に復帰。
// 画面の下方向が +Y なので、前進を +y にするため y を反転して返す。
// ───────────────────────────────────────────────────────────
const Joystick: React.FC<{
  size?: number;
  onChange: (x: number, y: number) => void;
}> = ({ size = 220, onChange }) => {
  const padRef = useRef<HTMLDivElement | null>(null);
  const [knob, setKnob] = useState({ x: 0, y: 0 }); // px offset for rendering
  const draggingRef = useRef(false);
  const radius = size / 2;
  const knobSize = size * 0.32;
  const maxR = radius - knobSize / 2;

  const update = useCallback(
    (clientX: number, clientY: number) => {
      const pad = padRef.current;
      if (!pad) return;
      const rect = pad.getBoundingClientRect();
      const cx = rect.left + rect.width / 2;
      const cy = rect.top + rect.height / 2;
      let dx = clientX - cx;
      let dy = clientY - cy;
      const dist = Math.hypot(dx, dy);
      if (dist > maxR) {
        dx = (dx / dist) * maxR;
        dy = (dy / dist) * maxR;
      }
      setKnob({ x: dx, y: dy });
      // 正規化 -1..1、yは反転(上=前進=+)
      onChange(
        +(dx / maxR).toFixed(3),
        +(-dy / maxR).toFixed(3)
      );
    },
    [maxR, onChange]
  );

  const release = useCallback(() => {
    draggingRef.current = false;
    setKnob({ x: 0, y: 0 });
    onChange(0, 0);
  }, [onChange]);

  const onPointerDown = (e: React.PointerEvent) => {
    draggingRef.current = true;
    (e.target as HTMLElement).setPointerCapture(e.pointerId);
    update(e.clientX, e.clientY);
  };
  const onPointerMove = (e: React.PointerEvent) => {
    if (!draggingRef.current) return;
    update(e.clientX, e.clientY);
  };

  return (
    <div
      ref={padRef}
      onPointerDown={onPointerDown}
      onPointerMove={onPointerMove}
      onPointerUp={release}
      onPointerCancel={release}
      style={{
        width: size,
        height: size,
        borderRadius: "50%",
        position: "relative",
        touchAction: "none",
        cursor: "grab",
        background:
          "radial-gradient(circle at 50% 50%, #1c2433 0%, #11161f 70%, #0c1017 100%)",
        border: "2px solid #2b3648",
        boxShadow: "inset 0 0 40px rgba(0,0,0,0.6)",
        userSelect: "none",
        flexShrink: 0,
      }}
    >
      {/* 十字ガイド */}
      <div style={{ position: "absolute", left: "50%", top: 8, bottom: 8, width: 1, background: "#2b3648", transform: "translateX(-0.5px)" }} />
      <div style={{ position: "absolute", top: "50%", left: 8, right: 8, height: 1, background: "#2b3648", transform: "translateY(-0.5px)" }} />
      {/* ノブ */}
      <div
        style={{
          position: "absolute",
          width: knobSize,
          height: knobSize,
          left: "50%",
          top: "50%",
          transform: `translate(calc(-50% + ${knob.x}px), calc(-50% + ${knob.y}px))`,
          borderRadius: "50%",
          background: "linear-gradient(145deg, #3b82f6, #1d4ed8)",
          boxShadow: "0 4px 12px rgba(0,0,0,0.5), inset 0 2px 4px rgba(255,255,255,0.25)",
          transition: draggingRef.current ? "none" : "transform 0.12s ease-out",
        }}
      />
    </div>
  );
};

const App: React.FC = () => {
  const ws = useRef<WebSocket | null>(null);

  const [url, setUrl] = useState<string>(DEFAULT_WS_URL);
  const [status, setStatus] = useState<ConnStatus>("disconnected");

  const [wheel, setWheel] = useState<Wheel>({ x: 0, y: 0, turn: 0 });
  const [servo, setServo] = useState<Servo>({ camera_left_right: 0, camera_up_down: 0 });
  const [sentJson, setSentJson] = useState<string>("{}");
  const [lastReply, setLastReply] = useState<string>("");

  // 送信ループが常に最新値を読めるよう ref に保持(再レンダ非依存)
  const stateRef = useRef<RobotState>({ wheel, servo });
  useEffect(() => {
    stateRef.current = { wheel, servo };
  }, [wheel, servo]);

  // ── 接続 ────────────────────────────────────────────────
  const connect = useCallback(() => {
    // 既存接続を畳む
    ws.current?.close();
    setStatus("connecting");
    try {
      const socket = new WebSocket(url);
      ws.current = socket;
      socket.onopen = () => setStatus("connected");
      socket.onclose = () => setStatus("disconnected");
      socket.onerror = () => setStatus("error");
      socket.onmessage = (ev) => setLastReply(String(ev.data));
    } catch {
      setStatus("error");
    }
  }, [url]);

  const disconnect = useCallback(() => {
    ws.current?.close();
    ws.current = null;
    setStatus("disconnected");
  }, []);

  // 初回だけ自動接続
  useEffect(() => {
    connect();
    return () => ws.current?.close();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  // ── 一定間隔で送信(20Hz) ──────────
  useEffect(() => {
    const id = setInterval(() => {
      const data = stateRef.current;
      const json = JSON.stringify(data);
      setSentJson(JSON.stringify(data, null, 2));
      if (ws.current?.readyState === WebSocket.OPEN) {
        ws.current.send(json);
      }
    }, SEND_INTERVAL_MS);
    return () => clearInterval(id);
  }, []);

  // ── UI ──────────────────────────────────────────────────
  const slider = (
    label: string,
    value: number,
    min: number,
    max: number,
    step: number,
    onChange: (v: number) => void
  ) => (
    <div style={{ marginBottom: 14 }}>
      <div style={{ display: "flex", justifyContent: "space-between", fontSize: 12, color: "#9aa6b8", marginBottom: 4 }}>
        <span>{label}</span>
        <span style={{ color: "#e2e8f0", fontVariantNumeric: "tabular-nums" }}>{value.toFixed(2)}</span>
      </div>
      <input
        type="range"
        min={min}
        max={max}
        step={step}
        value={value}
        onChange={(e) => onChange(parseFloat(e.target.value))}
        style={{ width: "100%", accentColor: "#3b82f6" }}
      />
    </div>
  );

  // 離すと 0 に戻るスライダー(旋回用)。
  // ドラッグ中は値を反映し、ポインタを離した時点で中央(0)へ復帰する。
  const springSlider = (
    label: string,
    value: number,
    min: number,
    max: number,
    step: number,
    onChange: (v: number) => void
  ) => {
    const reset = () => onChange(0);
    return (
      <div style={{ marginBottom: 14 }}>
        <div style={{ display: "flex", justifyContent: "space-between", fontSize: 12, color: "#9aa6b8", marginBottom: 4 }}>
          <span>{label}</span>
          <span style={{ color: "#e2e8f0", fontVariantNumeric: "tabular-nums" }}>{value.toFixed(2)}</span>
        </div>
        <input
          type="range"
          min={min}
          max={max}
          step={step}
          value={value}
          onChange={(e) => onChange(parseFloat(e.target.value))}
          onPointerUp={reset}
          onPointerCancel={reset}
          onMouseUp={reset}
          onTouchEnd={reset}
          style={{ width: "100%", accentColor: "#3b82f6" }}
        />
      </div>
    );
  };

  const meta = STATUS_META[status];
  const panel: React.CSSProperties = {
    background: "#0f141c",
    border: "1px solid #1e2733",
    borderRadius: 12,
    padding: 20,
  };
  const heading: React.CSSProperties = {
    margin: "0 0 16px",
    fontSize: 13,
  };

  return (
    <div
      style={{
        minHeight: "100vh",
        background: "#080b10",
        color: "#e2e8f0",
        fontFamily: "'JetBrains Mono', ui-monospace, 'SF Mono', Menlo, monospace",
        padding: 24,
        boxSizing: "border-box",
      }}
    >
      {/* ── 接続バー ── */}
      <div
        style={{
          ...panel,
          display: "flex",
          alignItems: "center",
          gap: 14,
          marginBottom: 20,
          flexWrap: "wrap",
        }}
      >
        <div style={{ display: "flex", alignItems: "center", gap: 8 }}>
          <span
            style={{
              width: 12,
              height: 12,
              borderRadius: "50%",
              background: meta.color,
              boxShadow: `0 0 10px ${meta.color}`,
              transition: "background 0.2s",
              animation: status === "connecting" ? "pulse 1s infinite" : undefined,
            }}
          />
          <span style={{ fontSize: 12, letterSpacing: "0.1em", color: meta.color, minWidth: 120 }}>
            {meta.label}
          </span>
        </div>

        <input
          value={url}
          onChange={(e) => setUrl(e.target.value)}
          spellCheck={false}
          placeholder="ws://host:port"
          style={{
            flex: 1,
            minWidth: 220,
            background: "#080b10",
            border: "1px solid #1e2733",
            borderRadius: 8,
            color: "#e2e8f0",
            padding: "10px 12px",
            fontFamily: "inherit",
            fontSize: 13,
            outline: "none",
          }}
        />

        <button onClick={connect} style={btn("#1d4ed8")}>
          {status === "connected" ? "RECONNECT" : "CONNECT"}
        </button>
        <button onClick={disconnect} style={btn("#334155")}>
          DISCONNECT
        </button>
      </div>

      {/* ── メイン3カラム ── */}
      <div
        style={{
          display: "grid",
          gridTemplateColumns: "minmax(260px, 1fr) minmax(220px, 1fr) minmax(220px, 1fr)",
          gap: 20,
          alignItems: "start",
        }}
      >
        {/* Wheel */}
        <div style={panel}>
          <h2 style={heading}>Wheel Control</h2>
          <div style={{ display: "flex", justifyContent: "center", marginBottom: 18 }}>
            <Joystick onChange={(x, y) => setWheel((w) => ({ ...w, x, y }))} />
          </div>
          <div style={{ display: "flex", gap: 16, justifyContent: "center", marginBottom: 16, fontSize: 12, color: "#9aa6b8" }}>
            <span>X <b style={{ color: "#e2e8f0" }}>{wheel.x.toFixed(2)}</b></span>
            <span>Y <b style={{ color: "#e2e8f0" }}>{wheel.y.toFixed(2)}</b></span>
          </div>
          {springSlider("Turn", wheel.turn, -1, 1, 0.01, (v) => setWheel((w) => ({ ...w, turn: v })))}
          <div style={{ fontSize: 11, color: "#475569", marginTop: 4 }}>
            パッド: 並進(X/Y) ・ スライダー: 旋回(Turn)
          </div>
        </div>

        {/* Servo */}
        <div style={panel}>
          <h2 style={heading}>Servo Control</h2>
          {slider("カメラ左右", servo.camera_left_right, -1, 1, 0.01, (v) => setServo((a) => ({ ...a, camera_left_right: v })))}
          {slider("カメラ上下", servo.camera_up_down, 0, 1, 0.01, (v) => setServo((a) => ({ ...a, camera_up_down: v })))}
        </div>

        {/* Telemetry */}
        <div style={panel}>
          <h2 style={heading}>Telemetry</h2>
          <div style={{ fontSize: 11, color: "#64748b", marginBottom: 6 }}>SENT ({Math.round(1000 / SEND_INTERVAL_MS)} Hz)</div>
          <pre
            style={{
              background: "#080b10",
              border: "1px solid #1e2733",
              padding: 12,
              borderRadius: 8,
              fontSize: 12,
              color: "#7dd3fc",
              overflowX: "auto",
              margin: 0,
              marginBottom: 16,
            }}
          >
            {sentJson}
          </pre>
          <div style={{ fontSize: 11, color: "#64748b", marginBottom: 6 }}>LAST REPLY</div>
          <div
            style={{
              background: "#080b10",
              border: "1px solid #1e2733",
              padding: 12,
              borderRadius: 8,
              fontSize: 12,
              color: lastReply.startsWith("[ NG") ? "#ef4444" : "#22c55e",
              minHeight: 18,
              wordBreak: "break-all",
            }}
          >
            {lastReply || "—"}
          </div>
        </div>
      </div>
      <iframe
        src="http://pibot.local:8080/html/p2p.html"
        style={{ width: "100%", height: 600, border: "none", borderRadius: 12 }}
        allow="autoplay"
      />
      <style>{`
        *{ padding: 0; margin:0 }
        @keyframes pulse { 0%,100% { opacity: 1 } 50% { opacity: 0.3 } }
        input[type=range] { height: 4px; -webkit-appearance: none; appearance: none; background: #1e2733; border-radius: 2px; }
        input[type=range]::-webkit-slider-thumb { -webkit-appearance: none; width: 16px; height: 16px; border-radius: 50%; background: #3b82f6; cursor: pointer; box-shadow: 0 0 8px rgba(59,130,246,0.5); }
        input[type=range]::-moz-range-thumb { width: 16px; height: 16px; border: none; border-radius: 50%; background: #3b82f6; cursor: pointer; }
      `}</style>
    </div>
  );
};

function btn(bg: string): React.CSSProperties {
  return {
    background: bg,
    color: "#fff",
    border: "none",
    borderRadius: 8,
    padding: "10px 18px",
    fontSize: 12,
    letterSpacing: "0.08em",
    fontFamily: "inherit",
    cursor: "pointer",
    fontWeight: 600,
  };
}

export default App;