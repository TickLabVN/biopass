import { invoke } from "@tauri-apps/api/core";
import { Fingerprint, Mic, ScanFace, ShieldCheck } from "lucide-react";
import { useEffect, useState } from "react";
import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select";
import { Switch } from "@/components/ui/switch";
import type {
  FaceMethodConfig,
  FingerprintMethodConfig,
  MethodsConfig,
  ModelConfig,
} from "@/types/config";
import { FaceCaptureSection } from "./methods/FaceCaptureSection";
import { FingerprintSection } from "./methods/FingerprintSection";
import { MethodCard } from "./methods/MethodCard";
import { ModelSelectField } from "./methods/shared/ModelSelectField";
import { SliderField } from "./methods/shared/SliderField";

interface Props {
  methods: MethodsConfig;
  models: ModelConfig[];
  onChange: (methods: MethodsConfig) => void;
}

export function MethodsSection({ methods, models, onChange }: Props) {
  const [expandedMethod, setExpandedMethod] = useState<string | null>("face");
  const [videoDevices, setVideoDevices] = useState<string[]>([]);

  useEffect(() => {
    const fetchDevices = async () => {
      try {
        const [vDevices] = await Promise.all([
          invoke<string[]>("list_video_devices"),
        ]);
        setVideoDevices(vDevices);
      } catch (err) {
        console.error("Failed to fetch devices:", err);
      }
    };
    fetchDevices();
  }, []);

  const updateFace = (face: FaceMethodConfig) => onChange({ ...methods, face });

  const updateFingerprint = (fingerprint: FingerprintMethodConfig) =>
    onChange({ ...methods, fingerprint });

  const methodIcons: Record<string, React.ReactNode> = {
    face: <ScanFace className="w-5 h-5 text-white" />,
    fingerprint: <Fingerprint className="w-5 h-5 text-white" />,
    voice: <Mic className="w-5 h-5 text-white" />,
  };

  const methodColors: Record<string, string> = {
    face: "from-violet-500 to-purple-500",
    fingerprint: "from-emerald-500 to-teal-500",
    voice: "from-orange-500 to-amber-500",
  };

  return (
    <div className="rounded-xl border border-border/50 bg-card/50 backdrop-blur-sm p-6 shadow-lg">
      <h2 className="text-xl font-semibold mb-4 flex items-center gap-2">
        <span className="w-8 h-8 rounded-lg bg-linear-to-br from-purple-500 to-pink-500 flex items-center justify-center">
          <ShieldCheck className="w-4 h-4 text-white" />
        </span>
        Authentication Methods
      </h2>

      <div className="grid gap-4">
        {/* Face Authentication */}
        <MethodCard
          title="Face Recognition"
          icon={methodIcons.face}
          color={methodColors.face}
          enabled={methods.face.enable}
          onToggle={(enable) => updateFace({ ...methods.face, enable })}
          expanded={expandedMethod === "face"}
          onExpand={() =>
            setExpandedMethod(expandedMethod === "face" ? null : "face")
          }
        >
          <div className="grid gap-4">
            {/* Retries and Delay */}
            <div className="grid grid-cols-2 gap-6 p-4 rounded-lg bg-muted/50 border border-border/50">
              <div className="grid gap-2">
                <Label
                  htmlFor="face-max-retries"
                  className="text-sm font-medium text-muted-foreground"
                >
                  Max Retries
                </Label>
                <Input
                  id="face-max-retries"
                  type="number"
                  min="0"
                  max="10"
                  value={methods.face.retries}
                  onChange={(e) =>
                    updateFace({
                      ...methods.face,
                      retries: parseInt(e.target.value, 10) || 0,
                    })
                  }
                  className="h-10"
                />
              </div>
              <div className="grid gap-2">
                <Label
                  htmlFor="face-retry-delay"
                  className="text-sm font-medium text-muted-foreground"
                >
                  Retry Delay (ms)
                </Label>
                <Input
                  id="face-retry-delay"
                  type="number"
                  min="0"
                  max="5000"
                  step="100"
                  value={methods.face.retry_delay}
                  onChange={(e) =>
                    updateFace({
                      ...methods.face,
                      retry_delay: parseInt(e.target.value, 10) || 0,
                    })
                  }
                  className="h-10"
                />
              </div>
            </div>

            {/* Face Capture */}
            <FaceCaptureSection />
            {/* Detection */}
            <div className="p-4 rounded-lg bg-muted/50 border border-border/50">
              <h4 className="font-medium mb-3 text-sm">Detection</h4>
              <div className="flex gap-6 items-end">
                <div className="flex-1 min-w-0">
                  <ModelSelectField
                    label="Model"
                    value={methods.face.detection.model}
                    models={models.filter((m) => m.type === "face")}
                    error={methods.face.enable}
                    onChange={(model) =>
                      updateFace({
                        ...methods.face,
                        detection: { ...methods.face.detection, model },
                      })
                    }
                  />
                </div>
                <div className="w-48 shrink-0">
                  <SliderField
                    label="Threshold"
                    value={methods.face.detection.threshold}
                    onChange={(threshold) =>
                      updateFace({
                        ...methods.face,
                        detection: { ...methods.face.detection, threshold },
                      })
                    }
                  />
                </div>
              </div>
            </div>
            {/* Recognition */}
            <div className="p-4 rounded-lg bg-muted/50 border border-border/50">
              <h4 className="font-medium mb-3 text-sm">Recognition</h4>
              <div className="flex gap-6 items-end">
                <div className="flex-1 min-w-0">
                  <ModelSelectField
                    label="Model"
                    value={methods.face.recognition.model}
                    models={models.filter((m) => m.type === "face")}
                    error={methods.face.enable}
                    onChange={(model) =>
                      updateFace({
                        ...methods.face,
                        recognition: { ...methods.face.recognition, model },
                      })
                    }
                  />
                </div>
                <div className="w-48 shrink-0">
                  <SliderField
                    label="Threshold"
                    value={methods.face.recognition.threshold}
                    onChange={(threshold) =>
                      updateFace({
                        ...methods.face,
                        recognition: { ...methods.face.recognition, threshold },
                      })
                    }
                  />
                </div>
              </div>
            </div>

            {/* Anti-Spoofing */}
            <div className="p-4 rounded-lg bg-muted/50 border border-border/50 space-y-3">
              <div className="flex items-center justify-between">
                <h4 className="font-medium text-sm">Anti-Spoofing</h4>
                <Switch
                  checked={methods.face.anti_spoofing.enable}
                  onCheckedChange={(enable) =>
                    updateFace({
                      ...methods.face,
                      anti_spoofing: { ...methods.face.anti_spoofing, enable },
                    })
                  }
                  className="cursor-pointer"
                />
              </div>
              {methods.face.anti_spoofing.enable && (
                <div className="flex gap-6 items-end">
                  <div className="flex-1 min-w-0">
                    <ModelSelectField
                      label="Model"
                      value={methods.face.anti_spoofing.model}
                      models={models.filter((m) => m.type === "face")}
                      error={methods.face.anti_spoofing.enable}
                      onChange={(model) =>
                        updateFace({
                          ...methods.face,
                          anti_spoofing: {
                            ...methods.face.anti_spoofing,
                            model,
                          },
                        })
                      }
                    />
                  </div>
                  <div className="w-48 shrink-0">
                    <SliderField
                      label="Threshold"
                      value={methods.face.anti_spoofing.threshold}
                      onChange={(threshold) =>
                        updateFace({
                          ...methods.face,
                          anti_spoofing: {
                            ...methods.face.anti_spoofing,
                            threshold,
                          },
                        })
                      }
                    />
                  </div>
                </div>
              )}
            </div>

            {/* IR Camera */}
            <div className="p-4 rounded-lg bg-muted/50 border border-border/50 space-y-3">
              <div className="flex items-center justify-between">
                <h4 className="font-medium text-sm">IR Camera</h4>
                <Switch
                  checked={methods.face.ir_camera.enable}
                  onCheckedChange={(enable) =>
                    updateFace({
                      ...methods.face,
                      ir_camera: { ...methods.face.ir_camera, enable },
                    })
                  }
                  className="cursor-pointer"
                />
              </div>
              {methods.face.ir_camera.enable && (
                <div className="grid gap-2">
                  <Label
                    htmlFor="ir-device"
                    className="text-xs text-muted-foreground"
                  >
                    Select Device
                  </Label>
                  <Select
                    value={
                      videoDevices.includes(
                        `/dev/video${methods.face.ir_camera.device_id}`,
                      )
                        ? `/dev/video${methods.face.ir_camera.device_id}`
                        : ""
                    }
                    onValueChange={(value) =>
                      updateFace({
                        ...methods.face,
                        ir_camera: {
                          ...methods.face.ir_camera,
                          device_id:
                            Number.parseInt(
                              value.replace("/dev/video", ""),
                              10,
                            ) || 0,
                        },
                      })
                    }
                  >
                    <SelectTrigger id="ir-device" className="h-10 w-full">
                      <SelectValue placeholder="Select IR Camera Device" />
                    </SelectTrigger>
                    <SelectContent>
                      {videoDevices.length > 0 ? (
                        videoDevices.map((device) => (
                          <SelectItem key={device} value={device}>
                            {device}
                          </SelectItem>
                        ))
                      ) : (
                        <SelectItem value="none" disabled>
                          No video devices found
                        </SelectItem>
                      )}
                    </SelectContent>
                  </Select>
                  <p className="text-[10px] text-muted-foreground">
                    Infrared camera device used for anti-spoofing.
                  </p>
                </div>
              )}
            </div>
          </div>
        </MethodCard>

        {/* Fingerprint Authentication */}
        <MethodCard
          title="Fingerprint"
          icon={methodIcons.fingerprint}
          color={methodColors.fingerprint}
          enabled={methods.fingerprint.enable}
          onToggle={(enable) =>
            updateFingerprint({ ...methods.fingerprint, enable })
          }
          expanded={expandedMethod === "fingerprint"}
          onExpand={() =>
            setExpandedMethod(
              expandedMethod === "fingerprint" ? null : "fingerprint",
            )
          }
        >
          <div className="grid gap-4 pt-4">
            {/* Retries and Delay */}
            <div className="grid grid-cols-2 gap-6 p-4 rounded-lg bg-muted/50 border border-border/50">
              <div className="grid gap-2">
                <Label
                  htmlFor="fingerprint-max-retries"
                  className="text-sm font-medium text-muted-foreground"
                >
                  Max Retries
                </Label>
                <Input
                  id="fingerprint-max-retries"
                  type="number"
                  min="0"
                  max="10"
                  value={methods.fingerprint.retries}
                  onChange={(e) =>
                    updateFingerprint({
                      ...methods.fingerprint,
                      retries: parseInt(e.target.value, 10) || 0,
                    })
                  }
                  className="h-10"
                />
              </div>
              <div className="grid gap-2">
                <Label
                  htmlFor="fingerprint-retry-delay"
                  className="text-sm font-medium text-muted-foreground"
                >
                  Retry Delay (ms)
                </Label>
                <Input
                  id="fingerprint-retry-delay"
                  type="number"
                  min="0"
                  max="5000"
                  step="100"
                  value={methods.fingerprint.retry_delay}
                  onChange={(e) =>
                    updateFingerprint({
                      ...methods.fingerprint,
                      retry_delay: parseInt(e.target.value, 10) || 0,
                    })
                  }
                  className="h-10"
                />
              </div>
            </div>

            <div className="overflow-hidden">
              <FingerprintSection
                config={methods.fingerprint}
                onUpdate={(fingerprint) => updateFingerprint(fingerprint)}
              />
            </div>
          </div>
        </MethodCard>

        {/* Voice Authentication — Coming Soon */}
        <MethodCard
          title="Voice Recognition"
          icon={methodIcons.voice}
          color={methodColors.voice}
          enabled={false}
          onToggle={() => {}}
          expanded={expandedMethod === "voice"}
          onExpand={() =>
            setExpandedMethod(expandedMethod === "voice" ? null : "voice")
          }
        >
          <div className="flex flex-col items-center justify-center gap-3 py-8 text-center">
            <div className="w-14 h-14 rounded-full bg-orange-500/10 flex items-center justify-center">
              <Mic className="w-7 h-7 text-orange-500" />
            </div>
            <div>
              <p className="font-semibold text-base">Coming Soon</p>
              <p className="text-sm text-muted-foreground mt-1 max-w-md">
                Voice recognition is currently under development and will be
                available in a future update.
              </p>
            </div>
            <span className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full text-xs font-medium bg-orange-500/10 text-orange-500 border border-orange-500/20">
              🚧 In Development
            </span>
          </div>
        </MethodCard>
      </div>
    </div>
  );
}
