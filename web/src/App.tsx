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

export const SUBSYSTEM_IDENTIFIER = "zmk__iqs9151";
const SUBSYSTEM_CANDIDATES = [SUBSYSTEM_IDENTIFIER, "iqs9151", "dya__iqs9151"];

type NumericField = keyof Pick<
  Iqs9151Config,
  | "resolutionX"
  | "resolutionY"
  | "atiTargetCount"
  | "dynamicFilterBottomSpeed"
  | "dynamicFilterTopSpeed"
  | "dynamicFilterBottomBeta"
>;

type BooleanField = keyof Pick<
  Iqs9151Config,
  | "oneFingerTap"
  | "twoFingerTap"
  | "threeFingerTap"
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
];

const booleanFields: Array<{ key: BooleanField; label: string; help: string }> = [
  { key: "oneFingerTap", label: "1F tap", help: "One-finger BTN0 tap" },
  { key: "twoFingerTap", label: "2F tap", help: "Two-finger BTN1 tap" },
  { key: "threeFingerTap", label: "3F tap", help: "Three-finger BTN2 tap" },
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
  twoFingerTap: true,
  threeFingerTap: true,
  scrollX: true,
  scrollY: true,
  pinch: true,
  cursorInertia: true,
  scrollInertia: true,
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
      setConfig(response.config?.config ?? null);
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
      setConfig(response.setConfig?.config ?? config);
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
      setConfig(response.resetConfig?.config ?? null);
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
          zmk__iqs9151 is not advertised. Available: {availableSubsystems.map(formatSubsystem).join(", ") || "none"}
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
