import { useState } from "react";
import { ChevronDown, ChevronRight, RotateCcw } from "lucide-react";
import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select";
import { Slider } from "@/components/ui/slider";
import type { AdvancedConfig } from "@/types/config";
import { DEFAULT_ADVANCED_CONFIG } from "@/types/config";

interface AdvancedFaceSettingsProps {
  advanced: AdvancedConfig;
  onChange: <K extends keyof AdvancedConfig>(key: K, value: AdvancedConfig[K]) => void;
  alwaysOpen?: boolean;
}

export function AdvancedFaceSettings({ advanced, onChange, alwaysOpen }: AdvancedFaceSettingsProps) {
  const [open, setOpen] = useState(alwaysOpen ?? false);

  const sections = (
    <div className="space-y-6">
      <div className="flex justify-end">
        <button
          type="button"
          onClick={() => {
            (Object.keys(DEFAULT_ADVANCED_CONFIG) as Array<keyof AdvancedConfig>).forEach((key) => {
              onChange(key, DEFAULT_ADVANCED_CONFIG[key]);
            });
          }}
          className="flex items-center gap-1.5 text-sm text-muted-foreground/60 hover:text-foreground transition-colors cursor-pointer"
          title="Reset all advanced settings to their default values"
        >
          <RotateCcw className="w-4 h-4" />
          Reset all to defaults
        </button>
      </div>
      <UnsharpMaskSection
        config={advanced.unsharp_mask}
        defaultConfig={DEFAULT_ADVANCED_CONFIG.unsharp_mask}
        onChange={(v) => onChange("unsharp_mask", v)}
      />
      <IrCaptureSection
        config={advanced.ir_capture}
        defaultConfig={DEFAULT_ADVANCED_CONFIG.ir_capture}
        onChange={(v) => onChange("ir_capture", v)}
      />
      <AntiSpoofingAdvancedSection
        config={advanced.anti_spoofing}
        defaultConfig={DEFAULT_ADVANCED_CONFIG.anti_spoofing}
        onChange={(v) => onChange("anti_spoofing", v)}
      />
      <DetectionAdvancedSection
        config={advanced.detection}
        defaultConfig={DEFAULT_ADVANCED_CONFIG.detection}
        onChange={(v) => onChange("detection", v)}
      />
      <CaptureResolutionSection
        config={advanced.capture}
        defaultConfig={DEFAULT_ADVANCED_CONFIG.capture}
        onChange={(v) => onChange("capture", v)}
      />
      <EnrollmentSection
        config={advanced.enrollment}
        defaultConfig={DEFAULT_ADVANCED_CONFIG.enrollment}
        onChange={(v) => onChange("enrollment", v)}
      />
      <RecognitionAdvancedSection
        config={advanced.recognition}
        defaultConfig={DEFAULT_ADVANCED_CONFIG.recognition}
        onChange={(v) => onChange("recognition", v)}
      />
      <AuthTimeoutSection
        config={advanced.auth}
        defaultConfig={DEFAULT_ADVANCED_CONFIG.auth}
        onChange={(v) => onChange("auth", v)}
      />
    </div>
  );

  if (alwaysOpen) {
    return sections;
  }

  return (
    <div className="p-4 rounded-lg bg-muted/50 border border-border/50 space-y-3">
      <button
        type="button"
        onClick={() => setOpen(!open)}
        className="flex items-center gap-2 w-full text-left font-medium text-sm cursor-pointer"
      >
        {open ? <ChevronDown className="w-4 h-4" /> : <ChevronRight className="w-4 h-4" />}
        Advanced Settings
      </button>
      {open && sections}
    </div>
  );
}

function UnsharpMaskSection({
  config,
  defaultConfig,
  onChange,
}: {
  config: AdvancedConfig["unsharp_mask"];
  defaultConfig: AdvancedConfig["unsharp_mask"];
  onChange: (v: AdvancedConfig["unsharp_mask"]) => void;
}) {
  return (
    <div className="space-y-3">
      <SectionHeading label="Unsharp Mask" onReset={() => onChange(defaultConfig)} />
      <p className="text-sm text-muted-foreground/60">
        Sharpens face crops before AI anti-spoofing to restore texture detail lost to webcam compression. Disable if using a model trained on soft images.
      </p>
      <div className="flex items-center gap-3">
        <label className="flex items-center gap-2 text-sm cursor-pointer">
          <input
            type="checkbox"
            checked={config.enable}
            onChange={(e) => onChange({ ...config, enable: e.target.checked })}
            className="cursor-pointer"
          />
          Enable sharpening
        </label>
      </div>
      {config.enable && (
        <div className="grid gap-2 max-w-48">
          <div className="flex items-center justify-between">
            <Label className="text-sm text-muted-foreground">Amount</Label>
            <span className="text-sm font-mono">{config.amount.toFixed(1)}</span>
          </div>
          <Slider
            value={[config.amount]}
            min={0}
            max={15}
            step={0.5}
            onValueChange={([v]) => onChange({ ...config, amount: v })}
          />
        </div>
      )}
    </div>
  );
}

function IrCaptureSection({
  config,
  defaultConfig,
  onChange,
}: {
  config: AdvancedConfig["ir_capture"];
  defaultConfig: AdvancedConfig["ir_capture"];
  onChange: (v: AdvancedConfig["ir_capture"]) => void;
}) {
  return (
    <div className="space-y-3">
      <SectionHeading label="IR Camera Capture" onReset={() => onChange(defaultConfig)} />
      <p className="text-sm text-muted-foreground/60">
        Timing and retry behavior for IR frame capture. AGC sleep lets the camera's auto-gain control stabilise between attempts.
      </p>
      <div className="grid grid-cols-3 gap-3">
        <NumberInput label="Warmup Frames" value={config.warmup_frames} min={0} max={100} title="Frames to discard after opening camera (flushes stale buffered frames)" onChange={(v) => onChange({ ...config, warmup_frames: v })} />
        <NumberInput label="Timeout (ms)" value={config.capture_timeout_ms} min={0} max={30000} step={100} title="Max time to wait for a valid IR frame before giving up" onChange={(v) => onChange({ ...config, capture_timeout_ms: v })} />
        <NumberInput label="Poll (ms)" value={config.poll_interval_ms} min={1} max={1000} fallback={33} title="Interval between frame availability checks during capture" onChange={(v) => onChange({ ...config, poll_interval_ms: v })} />
        <NumberInput label="Max Attempts" value={config.max_attempts} min={1} max={10} fallback={1} title="Full capture+detect retries before failing" onChange={(v) => onChange({ ...config, max_attempts: v })} />
        <NumberInput label="AGC Sleep (ms)" value={config.agc_sleep_ms} min={0} max={10000} step={100} title="Pause between attempts for IR auto-gain-control to stabilise" onChange={(v) => onChange({ ...config, agc_sleep_ms: v })} />
        <NumberInput label="Cam Warmup (ms)" value={config.camera_warmup_ms} min={0} max={10000} step={100} title="Initial delay after opening camera before any capture attempt" onChange={(v) => onChange({ ...config, camera_warmup_ms: v })} />
      </div>
    </div>
  );
}

function AntiSpoofingAdvancedSection({
  config,
  defaultConfig,
  onChange,
}: {
  config: AdvancedConfig["anti_spoofing"];
  defaultConfig: AdvancedConfig["anti_spoofing"];
  onChange: (v: AdvancedConfig["anti_spoofing"]) => void;
}) {
  return (
    <div className="space-y-3">
      <SectionHeading label="Anti-Spoofing Advanced" onReset={() => onChange(defaultConfig)} />
      <p className="text-sm text-muted-foreground/60">
        Spoof class index: which model output class means "spoof" (0 or 1). Combinational mode: "all must pass" (strict) or "any must pass" (relaxed). Debug save path: where failed auth images are saved.
      </p>
      <div className="grid grid-cols-3 gap-3">
        <NumberInput label="Spoof Class Index" value={config.spoof_class} min={0} max={10} title="Which model output class index corresponds to spoof attack" onChange={(v) => onChange({ ...config, spoof_class: v })} />
        <div className="grid gap-1">
          <Label className="text-sm text-muted-foreground" title="Whether all anti-spoofing methods must pass, or any one is sufficient">Combinational Mode</Label>
          <Select
            value={config.combinational_mode}
            onValueChange={(v) => onChange({ ...config, combinational_mode: v as "all" | "any" })}
          >
            <SelectTrigger className="h-10 text-sm">
              <SelectValue />
            </SelectTrigger>
            <SelectContent>
              <SelectItem value="all">All must pass</SelectItem>
              <SelectItem value="any">Any must pass</SelectItem>
            </SelectContent>
          </Select>
        </div>
        <div className="grid gap-1">
          <Label className="text-sm text-muted-foreground" title="Directory for saving failed auth debug images (empty = default)">Debug Save Path</Label>
          <Input
            type="text"
            value={config.debug_save_path}
            onChange={(e) => onChange({ ...config, debug_save_path: e.target.value })}
            placeholder="(default)"
            className="h-10 text-sm"
          />
        </div>
      </div>
    </div>
  );
}

function DetectionAdvancedSection({
  config,
  defaultConfig,
  onChange,
}: {
  config: AdvancedConfig["detection"];
  defaultConfig: AdvancedConfig["detection"];
  onChange: (v: AdvancedConfig["detection"]) => void;
}) {
  return (
    <div className="space-y-3">
      <SectionHeading label="Detection Advanced" onReset={() => onChange(defaultConfig)} />
      <p className="text-sm text-muted-foreground/60">
        YOLOv8 input size (higher = better accuracy, slower). NMS IoU threshold for merging overlapping face boxes — lower = less overlap tolerance.
      </p>
      <div className="grid grid-cols-2 gap-3">
        <NumberInput label="Input Size" value={config.input_size} min={160} max={1920} step={32} fallback={640} title="YOLOv8 network input dimension (width = height)" onChange={(v) => onChange({ ...config, input_size: v })} />
        <div className="grid gap-1">
          <div className="flex items-center justify-between">
            <Label className="text-sm text-muted-foreground" title="Overlap threshold for merging duplicate face boxes (lower = stricter)">NMS IoU</Label>
            <span className="text-sm font-mono">{(config.nms_iou_threshold * 100).toFixed(0)}%</span>
          </div>
          <div className="h-10 flex items-center">
            <Slider
              value={[config.nms_iou_threshold]}
              min={0}
              max={1}
              step={0.01}
              onValueChange={([v]) => onChange({ ...config, nms_iou_threshold: v })}
            />
          </div>
        </div>
      </div>
    </div>
  );
}

function CaptureResolutionSection({
  config,
  defaultConfig,
  onChange,
}: {
  config: AdvancedConfig["capture"];
  defaultConfig: AdvancedConfig["capture"];
  onChange: (v: AdvancedConfig["capture"]) => void;
}) {
  return (
    <div className="space-y-3">
      <SectionHeading label="Capture Resolution" onReset={() => onChange(defaultConfig)} />
      <p className="text-sm text-muted-foreground/60">
        Frame width/height for the RGB camera. Higher = better detection at distance but slower. Preview FPS is controlled from the camera preview bar above.
      </p>
      <div className="grid grid-cols-2 gap-3">
        <NumberInput label="Width" value={config.width} min={160} max={3840} step={16} fallback={640} title="Capture frame width in pixels" onChange={(v) => onChange({ ...config, width: v })} />
        <NumberInput label="Height" value={config.height} min={120} max={2160} step={16} fallback={480} title="Capture frame height in pixels" onChange={(v) => onChange({ ...config, height: v })} />
      </div>
    </div>
  );
}

function EnrollmentSection({
  config,
  defaultConfig,
  onChange,
}: {
  config: AdvancedConfig["enrollment"];
  defaultConfig: AdvancedConfig["enrollment"];
  onChange: (v: AdvancedConfig["enrollment"]) => void;
}) {
  return (
    <div className="space-y-3">
      <SectionHeading label="Enrollment" onReset={() => onChange(defaultConfig)} />
      <p className="text-sm text-muted-foreground/60">
        Number of frames to capture when enrolling a new face. More frames improve recognition robustness at the cost of slower enrollment.
      </p>
      <div className="grid grid-cols-1 gap-3 max-w-32">
        <NumberInput label="Capture Count" value={config.capture_count} min={1} max={20} fallback={1} title="Number of face frames to capture during enrollment" onChange={(v) => onChange({ ...config, capture_count: v })} />
      </div>
    </div>
  );
}

function RecognitionAdvancedSection({
  config,
  defaultConfig,
  onChange,
}: {
  config: AdvancedConfig["recognition"];
  defaultConfig: AdvancedConfig["recognition"];
  onChange: (v: AdvancedConfig["recognition"]) => void;
}) {
  return (
    <div className="space-y-3">
      <SectionHeading label="Recognition Advanced" onReset={() => onChange(defaultConfig)} />
      <p className="text-sm text-muted-foreground/60">
        Custom directory for enrolled face images. Leave empty to use the default data directory.
      </p>
      <div className="grid grid-cols-1 gap-3">
        <div className="grid gap-1">
          <Label className="text-sm text-muted-foreground" title="Custom directory for storing enrolled face images (empty = default)">Gallery Path</Label>
          <Input
            type="text"
            value={config.gallery_path}
            onChange={(e) => onChange({ ...config, gallery_path: e.target.value })}
            placeholder="(default data dir)"
            className="h-10 text-sm"
          />
        </div>
      </div>
    </div>
  );
}

function AuthTimeoutSection({
  config,
  defaultConfig,
  onChange,
}: {
  config: AdvancedConfig["auth"];
  defaultConfig: AdvancedConfig["auth"];
  onChange: (v: AdvancedConfig["auth"]) => void;
}) {
  return (
    <div className="space-y-3">
      <SectionHeading label="Authentication Timeout" onReset={() => onChange(defaultConfig)} />
      <p className="text-sm text-muted-foreground/60">
        Hard cap on total authentication time in milliseconds. 0 = auto (computed from retries × retry delay + 5s margin).
      </p>
      <div className="grid grid-cols-1 gap-3 max-w-48">
        <NumberInput label="Max Auth Time (ms, 0 = auto)" value={config.max_time_ms} min={0} max={60000} step={100} fallback={0} title="Absolute timeout for entire authentication (0 = auto from retries \u00d7 delay + margin)" onChange={(v) => onChange({ ...config, max_time_ms: v })} />
      </div>
    </div>
  );
}

function SectionHeading({ label, onReset }: { label: string; onReset: () => void }) {
  return (
    <h5 className="text-sm font-medium text-muted-foreground flex items-center gap-2">
      <span>{label}</span>
      <button
        type="button"
        onClick={onReset}
        className="inline-flex items-center text-muted-foreground/30 hover:text-foreground transition-colors cursor-pointer"
        title="Reset this section to defaults"
      >
        <RotateCcw className="w-4 h-4" />
      </button>
    </h5>
  );
}

function NumberInput({
  label,
  value,
  min,
  max,
  step,
  fallback,
  title,
  onChange,
}: {
  label: string;
  value: number;
  min: number;
  max: number;
  step?: number;
  fallback?: number;
  title?: string;
  onChange: (v: number) => void;
}) {
  return (
    <div className="grid gap-1">
      <Label className="text-sm text-muted-foreground" title={title}>{label}</Label>
      <Input
        type="number"
        min={min}
        max={max}
        step={step ?? 1}
        value={value}
        onChange={(e) => onChange(parseInt(e.target.value, 10) || (fallback ?? 0))}
        className="h-10 text-sm"
      />
    </div>
  );
}
