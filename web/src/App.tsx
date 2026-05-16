import { useContext, useMemo, useState } from "react";
import "./App.css";
import { connect as serialConnect } from "@zmkfirmware/zmk-studio-ts-client/transport/serial";
import { connect as gattConnect } from "@zmkfirmware/zmk-studio-ts-client/transport/gatt";
import {
  ZMKAppContext,
  ZMKConnection,
  ZMKCustomSubsystem,
} from "@cormoran/zmk-studio-react-hook";
import {
  Iqs9151Config,
  Request,
  Response,
} from "./proto/zmk/iqs9151/iqs9151";

export const SUBSYSTEM_IDENTIFIER = "dya__iqs9151";
const SUBSYSTEM_CANDIDATES = [SUBSYSTEM_IDENTIFIER, "zmk__iqs9151", "iqs9151"];

type NumericField = keyof Pick<
  Iqs9151Config,
  | "resolutionX"
  | "resolutionY"
  | "atiTargetCount"
  | "dynamicFilterBottomSpeed"
  | "dynamicFilterTopSpeed"
  | "dynamicFilterBottomBeta"
  | "oneFingerTapMaxMs"
  | "oneFingerTapMove"
  | "oneFingerTapdragGapMaxMs"
  | "twoFingerTapMaxMs"
  | "twoFingerTapMove"
  | "twoFingerTapdragGapMaxMs"
  | "threeFingerTapMaxMs"
  | "threeFingerTapMove"
  | "threeFingerTapdragGapMaxMs"
  | "threeFingerSwipeThreshold"
  | "twoFingerScrollStartMove"
  | "twoFingerPinchStartDistance"
  | "twoFingerPinchWheelGainX10"
  | "cursorInertiaDecay"
  | "cursorInertiaRecentWindowMs"
  | "cursorInertiaStaleGapMs"
  | "cursorInertiaMinSamples"
  | "cursorInertiaMinAvgSpeed"
  | "scrollInertiaDecay"
  | "scrollInertiaRecentWindowMs"
  | "scrollInertiaStaleGapMs"
  | "scrollInertiaMinSamples"
  | "scrollInertiaMinAvgSpeed"
>;

type BooleanField = keyof Pick<
  Iqs9151Config,
  | "oneFingerTap"
  | "oneFingerPresshold"
  | "twoFingerTap"
  | "twoFingerPresshold"
  | "threeFingerTap"
  | "threeFingerPresshold"
  | "scrollX"
  | "scrollY"
  | "pinch"
  | "cursorInertia"
  | "scrollInertia"
>;

const numericFields: Array<{
  key: NumericField;
  label: string;
  min: number;
  max: number;
  help: string;
}> = [
  { key: "resolutionX", label: "X resolution", min: 0, max: 4095, help: "IQS9151 X coordinate scale" },
  { key: "resolutionY", label: "Y resolution", min: 0, max: 4095, help: "IQS9151 Y coordinate scale" },
  { key: "atiTargetCount", label: "ATI target", min: 0, max: 1000, help: "Trackpad ATI target count" },
  { key: "dynamicFilterBottomSpeed", label: "Filter bottom speed", min: 0, max: 2047, help: "XY dynamic filter lower speed threshold" },
  { key: "dynamicFilterTopSpeed", label: "Filter top speed", min: 0, max: 2047, help: "XY dynamic filter upper speed threshold" },
  { key: "dynamicFilterBottomBeta", label: "Filter bottom beta", min: 0, max: 255, help: "XY dynamic filter beta value" },
  { key: "oneFingerTapMaxMs", label: "1F tap max ms", min: 1, max: 1000, help: "Maximum duration for one-finger tap" },
  { key: "oneFingerTapMove", label: "1F tap move", min: 1, max: 1000, help: "Move tolerance for one-finger tap" },
  { key: "oneFingerTapdragGapMaxMs", label: "1F tap-drag gap", min: 1, max: 1000, help: "Maximum gap before one-finger tap-drag" },
  { key: "twoFingerTapMaxMs", label: "2F tap max ms", min: 1, max: 1000, help: "Maximum duration for two-finger tap" },
  { key: "twoFingerTapMove", label: "2F tap move", min: 1, max: 1000, help: "Move tolerance for two-finger tap" },
  { key: "twoFingerTapdragGapMaxMs", label: "2F tap-drag gap", min: 1, max: 1000, help: "Maximum gap before two-finger tap-drag" },
  { key: "threeFingerTapMaxMs", label: "3F tap max ms", min: 1, max: 1000, help: "Maximum duration for three-finger tap" },
  { key: "threeFingerTapMove", label: "3F tap move", min: 1, max: 1000, help: "Move tolerance for three-finger tap" },
  { key: "threeFingerTapdragGapMaxMs", label: "3F tap-drag gap", min: 1, max: 1000, help: "Maximum gap before three-finger tap-drag" },
  { key: "threeFingerSwipeThreshold", label: "3F swipe threshold", min: 0, max: 1000, help: "Distance threshold for three-finger swipe" },
  { key: "twoFingerScrollStartMove", label: "2F scroll start", min: 1, max: 2000, help: "Movement before two-finger scroll starts" },
  { key: "twoFingerPinchStartDistance", label: "Pinch start distance", min: 1, max: 2000, help: "Distance change before pinch starts" },
  { key: "twoFingerPinchWheelGainX10", label: "Pinch gain x10", min: 1, max: 100, help: "Pinch wheel gain in tenths" },
  { key: "cursorInertiaDecay", label: "Cursor inertia decay", min: 0, max: 1000, help: "Cursor inertia decay numerator" },
  { key: "cursorInertiaRecentWindowMs", label: "Cursor recent window", min: 1, max: 500, help: "Recent motion window for cursor inertia" },
  { key: "cursorInertiaStaleGapMs", label: "Cursor stale gap", min: 1, max: 500, help: "Maximum gap between cursor samples" },
  { key: "cursorInertiaMinSamples", label: "Cursor min samples", min: 1, max: 12, help: "Minimum samples to seed cursor inertia" },
  { key: "cursorInertiaMinAvgSpeed", label: "Cursor min speed", min: 1, max: 500, help: "Minimum average speed for cursor inertia" },
  { key: "scrollInertiaDecay", label: "Scroll inertia decay", min: 0, max: 1000, help: "Scroll inertia decay numerator" },
  { key: "scrollInertiaRecentWindowMs", label: "Scroll recent window", min: 1, max: 500, help: "Recent motion window for scroll inertia" },
  { key: "scrollInertiaStaleGapMs", label: "Scroll stale gap", min: 1, max: 500, help: "Maximum gap between scroll samples" },
  { key: "scrollInertiaMinSamples", label: "Scroll min samples", min: 1, max: 12, help: "Minimum samples to seed scroll inertia" },
  { key: "scrollInertiaMinAvgSpeed", label: "Scroll min speed", min: 1, max: 500, help: "Minimum average speed for scroll inertia" },
];

const booleanFields: Array<{ key: BooleanField; label: string; help: string }> = [
  { key: "oneFingerTap", label: "1F tap", help: "One-finger BTN0 tap" },
  { key: "oneFingerPresshold", label: "1F press-hold", help: "Hold BTN0 for one-finger press" },
  { key: "twoFingerTap", label: "2F tap", help: "Two-finger BTN1 tap" },
  { key: "twoFingerPresshold", label: "2F press-hold", help: "Hold BTN1 for two-finger press" },
  { key: "threeFingerTap", label: "3F tap", help: "Three-finger BTN2 tap" },
  { key: "threeFingerPresshold", label: "3F press-hold", help: "Hold BTN2 for three-finger press" },
  { key: "scrollX", label: "Horizontal scroll", help: "Two-finger REL_HWHEEL" },
  { key: "scrollY", label: "Vertical scroll", help: "Two-finger REL_WHEEL" },
  { key: "pinch", label: "Pinch", help: "Two-finger pinch wheel" },
  { key: "cursorInertia", label: "Cursor inertia", help: "Continue cursor motion after release" },
  { key: "scrollInertia", label: "Scroll inertia", help: "Continue scroll motion after release" },
];

const demoConfig: Iqs9151Config = {
  resolutionX: 2457,
  resolutionY: 3072,
  atiTargetCount: 400,
  dynamicFilterBottomSpeed: 30,
  dynamicFilterTopSpeed: 511,
  dynamicFilterBottomBeta: 20,
  oneFingerTap: true,
  oneFingerPresshold: false,
  oneFingerTapMaxMs: 120,
  oneFingerTapMove: 25,
  oneFingerTapdragGapMaxMs: 230,
  twoFingerTap: true,
  twoFingerPresshold: false,
  twoFingerTapMaxMs: 130,
  twoFingerTapMove: 30,
  twoFingerTapdragGapMaxMs: 200,
  threeFingerTap: true,
  threeFingerPresshold: false,
  threeFingerTapMaxMs: 180,
  threeFingerTapMove: 30,
  threeFingerTapdragGapMaxMs: 230,
  threeFingerSwipeThreshold: 300,
  scrollX: true,
  scrollY: true,
  twoFingerScrollStartMove: 50,
  pinch: true,
  twoFingerPinchStartDistance: 80,
  twoFingerPinchWheelGainX10: 40,
  cursorInertia: true,
  cursorInertiaDecay: 970,
  cursorInertiaRecentWindowMs: 60,
  cursorInertiaStaleGapMs: 35,
  cursorInertiaMinSamples: 2,
  cursorInertiaMinAvgSpeed: 10,
  scrollInertia: true,
  scrollInertiaDecay: 985,
  scrollInertiaRecentWindowMs: 60,
  scrollInertiaStaleGapMs: 35,
  scrollInertiaMinSamples: 1,
  scrollInertiaMinAvgSpeed: 4,
};

function App() {
  const [demoMode, setDemoMode] = useState(false);

  return (
    <div className="app-shell">
      <header className="hero">
        <p className="eyebrow">DYA Studio Subsystem</p>
        <h1>IQS9151 Touchpad Tuning</h1>
        <p className="lead">Polaris IQS shieldのタッチパッド設定をStudio Custom RPCで調整します。</p>
        <button className="ghost" onClick={() => setDemoMode((value) => !value)}>
          {demoMode ? "Disable Demo" : "Enable Demo"}
        </button>
      </header>

      <ZMKConnection
        renderDisconnected={({ connect, isLoading, error }) => (
          <>
            <section className="panel connection">
              <h2>Connection</h2>
              {isLoading && <p>Connecting...</p>}
              {error && <p className="error">{error}</p>}
              {!isLoading && (
                <div className="actions">
                  <button className="primary" onClick={() => connect(gattConnect)}>
                    Connect Bluetooth
                  </button>
                  <button className="secondary" onClick={() => connect(serialConnect)}>
                    Connect Serial
                  </button>
                </div>
              )}
            </section>
            {demoMode && <TuningPanel demoMode />}
          </>
        )}
        renderConnected={({ disconnect, deviceName }) => (
          <>
            <section className="panel connection connected">
              <div>
                <h2>Connected</h2>
                <p>{deviceName}</p>
              </div>
              <button className="secondary" onClick={disconnect}>
                Disconnect
              </button>
            </section>
            <TuningPanel demoMode={demoMode} />
          </>
        )}
      />
    </div>
  );
}

function TuningPanel({ demoMode = false }: { demoMode?: boolean }) {
  const zmkApp = useContext(ZMKAppContext);
  const [config, setConfig] = useState<Iqs9151Config | null>(demoMode ? demoConfig : null);
  const [status, setStatus] = useState(demoMode ? "Demo config loaded" : "");
  const [busy, setBusy] = useState(false);

  const subsystem = useMemo(() => {
    if (!zmkApp || demoMode) return null;
    for (const candidate of SUBSYSTEM_CANDIDATES) {
      const found = zmkApp.findSubsystem(candidate);
      if (found) return found;
    }
    return null;
  }, [zmkApp, demoMode]);

  const availableSubsystems = ((zmkApp?.state.connection as any)?.subsystems ?? []) as Array<Record<string, unknown>>;

  const callRPC = async (request: Request) => {
    if (!zmkApp?.state.connection || !subsystem) {
      throw new Error("IQS9151 subsystem is not available");
    }
    const service = new ZMKCustomSubsystem(zmkApp.state.connection, subsystem.index);
    const payload = Request.encode(Request.create(request)).finish();
    const responsePayload = await service.callRPC(payload);
    if (!responsePayload) throw new Error("Empty RPC response");
    const response = Response.decode(responsePayload);
    if (response.error) throw new Error(response.error.message || "RPC error");
    return response;
  };

  const loadConfig = async () => {
    if (demoMode) {
      setConfig(demoConfig);
      setStatus("Demo config loaded");
      return;
    }
    setBusy(true);
    setStatus("");
    try {
      const response = await callRPC({ getConfig: {} });
      const loadedConfig = response.config?.config;
      if (!loadedConfig) {
        throw new Error("IQS9151 config response did not include parameters");
      }
      setConfig(loadedConfig);
      setStatus("Loaded current IQS9151 config");
    } catch (error) {
      setStatus(error instanceof Error ? error.message : "Failed to load config");
    } finally {
      setBusy(false);
    }
  };

  const applyConfig = async () => {
    if (!config) return;
    if (demoMode) {
      setStatus("Demo config applied locally");
      return;
    }
    setBusy(true);
    setStatus("");
    try {
      const response = await callRPC({ setConfig: { config } });
      const appliedConfig = response.setConfig?.config;
      if (!appliedConfig) {
        throw new Error("IQS9151 apply response did not include parameters");
      }
      setConfig(appliedConfig);
      setStatus("Applied IQS9151 config");
    } catch (error) {
      setStatus(error instanceof Error ? error.message : "Failed to apply config");
    } finally {
      setBusy(false);
    }
  };

  const resetConfig = async () => {
    if (demoMode) {
      setConfig(demoConfig);
      setStatus("Demo config reset");
      return;
    }
    setBusy(true);
    setStatus("");
    try {
      const response = await callRPC({ resetConfig: {} });
      const resetConfig = response.resetConfig?.config;
      if (!resetConfig) {
        throw new Error("IQS9151 reset response did not include parameters");
      }
      setConfig(resetConfig);
      setStatus("Reset IQS9151 config to firmware defaults");
    } catch (error) {
      setStatus(error instanceof Error ? error.message : "Failed to reset config");
    } finally {
      setBusy(false);
    }
  };

  const updateNumber = (key: NumericField, value: number) => {
    setConfig((current) => (current ? { ...current, [key]: value } : current));
  };

  const updateBoolean = (key: BooleanField, value: boolean) => {
    setConfig((current) => (current ? { ...current, [key]: value } : current));
  };

  const canUseSubsystem = demoMode || Boolean(subsystem);

  return (
    <section className="panel tuner">
      <div className="panel-head">
        <div>
          <p className="eyebrow">Subsystem</p>
          <h2>IQS9151 Parameters</h2>
        </div>
        <span className={canUseSubsystem ? "badge ok" : "badge warn"}>
          {demoMode ? "demo" : subsystem ? formatSubsystem(subsystem as unknown as Record<string, unknown>) : "not found"}
        </span>
      </div>

      {!demoMode && zmkApp?.state.connection && !subsystem && (
        <p className="warning">
          dya__iqs9151 is not advertised. Available: {availableSubsystems.map(formatSubsystem).join(", ") || "none"}
        </p>
      )}

      <div className="actions">
        <button className="primary" disabled={busy || !canUseSubsystem} onClick={loadConfig}>
          Load
        </button>
        <button className="primary" disabled={busy || !config || !canUseSubsystem} onClick={applyConfig}>
          Apply
        </button>
        <button className="secondary" disabled={busy || !canUseSubsystem} onClick={resetConfig}>
          Reset Defaults
        </button>
      </div>

      {status && <p className="status">{status}</p>}

      {config && (
        <>
          <div className="grid numeric-grid">
            {numericFields.map((field) => (
              <label className="field" key={field.key}>
                <span>{field.label}</span>
                <small>{field.help}</small>
                <input
                  type="number"
                  min={field.min}
                  max={field.max}
                  value={Number(config[field.key] ?? 0)}
                  onChange={(event) => updateNumber(field.key, Number(event.target.value))}
                />
              </label>
            ))}
          </div>

          <div className="toggle-grid">
            {booleanFields.map((field) => (
              <label className="toggle" key={field.key}>
                <input
                  type="checkbox"
                  checked={Boolean(config[field.key])}
                  onChange={(event) => updateBoolean(field.key, event.target.checked)}
                />
                <span>
                  <strong>{field.label}</strong>
                  <small>{field.help}</small>
                </span>
              </label>
            ))}
          </div>

          <pre>{JSON.stringify(config, null, 2)}</pre>
        </>
      )}
    </section>
  );
}

function formatSubsystem(subsystem: Record<string, unknown>) {
  const id = subsystem.identifier ?? subsystem.id ?? subsystem.name ?? "unknown";
  const index = subsystem.index;
  return index == null ? String(id) : String(id) + "#" + String(index);
}

export default App;
