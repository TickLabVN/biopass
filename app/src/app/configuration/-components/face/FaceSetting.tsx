import { useEffect, useMemo, useState } from "react";
import { cmd } from "@/commands";
import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select";
import type { AdvancedConfig, FaceMethodConfig, VideoDeviceInfo } from "@/types/config";
import { useConfigurationStore } from "../../-stores/configuration-store";
import { ModelSelect } from "../methods/shared/ModelSelect";
import { Threshold } from "../methods/shared/Threshold";
import { FaceCapture } from "./FaceCapture";

export function FaceSetting() {
  const config = useConfigurationStore((state) => state.config?.methods.face);
  const models = useConfigurationStore((state) => state.config?.models ?? []);
  const setFaceConfig = useConfigurationStore((state) => state.setFaceConfig);
  const [videoDevices, setVideoDevices] = useState<VideoDeviceInfo[]>([]);

  useEffect(() => {
    const fetchDevices = async () => {
      try {
        setVideoDevices(await cmd.face.listVideoDevices());
      } catch (err) {
        console.error("Failed to fetch devices:", err);
      }
    };

    fetchDevices();
  }, []);

  const selectedCameraDevice = useMemo(() => {
    if (!config) return null;
    const cameraDevice = config.camera_device;
    if (!cameraDevice) return null;
    return videoDevices.find((device) => device.path === cameraDevice) ?? null;
  }, [config, videoDevices]);

  const selectedIrCamera = useMemo(() => {
    if (!config) return null;
    const irCameraPath = config.anti_spoofing.ir_camera;
    if (!irCameraPath) return null;
    return videoDevices.find((device) => device.path === irCameraPath) ?? null;
  }, [config, videoDevices]);
  const antiSpoofModels = useMemo(
    () => models.filter((m) => m.type === "anti-spoofing"),
    [models],
  );

  if (!config) return null;

  const faceConfig: FaceMethodConfig = config;

  function setAdvancedField<K extends keyof AdvancedConfig>(key: K, value: AdvancedConfig[K]) {
    setFaceConfig({ ...faceConfig, advanced: { ...faceConfig.advanced, [key]: value } } as FaceMethodConfig);
  }

  const disabledOption = "__disabled__";
  const unavailableAiModelOption = "__unavailable_ai_model__";
  const unavailableIrDeviceOption = "__unavailable_ir_device__";
  const selectedAiModelExists = antiSpoofModels.some(
    (model) => model.path === config.anti_spoofing.model.path,
  );
  const irCameraValue = config.anti_spoofing.ir_camera
    ? (selectedIrCamera?.path ?? unavailableIrDeviceOption)
    : disabledOption;
  const aiModelValue = config.anti_spoofing.enable
    ? selectedAiModelExists
      ? config.anti_spoofing.model.path
      : unavailableAiModelOption
    : disabledOption;

  return (
    <div className="grid gap-4">
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
            value={config.retries}
            onChange={(e) =>
              setFaceConfig({
                ...config,
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
            value={config.retry_delay}
            onChange={(e) =>
              setFaceConfig({
                ...config,
                retry_delay: parseInt(e.target.value, 10) || 0,
              })
            }
            className="h-10"
          />
        </div>
      </div>

      <div className="grid gap-2">
        <Label htmlFor="camera-device" className="text-xs text-muted-foreground">
          Camera Device
        </Label>
        <Select
          value={config.camera_device ?? "__default__"}
          onValueChange={(value) => {
            setFaceConfig({
              ...config,
              camera_device: value === "__default__" ? null : value,
            });
          }}
        >
          <SelectTrigger id="camera-device" className="h-10 w-full">
            <SelectValue placeholder="Select Camera Device" />
          </SelectTrigger>
          <SelectContent>
            <SelectItem value="__default__">Default (auto)</SelectItem>
            {videoDevices.map((device) => (
              <SelectItem key={device.path} value={device.path}>
                {device.display_name}
              </SelectItem>
            ))}
          </SelectContent>
        </Select>
      </div>

      <FaceCapture
        selectedDevicePath={selectedCameraDevice?.path}
        previewFps={config.advanced.capture.preview_fps}
        onPreviewFpsChange={(fps) =>
          setAdvancedField("capture", {
            ...config.advanced.capture,
            preview_fps: fps,
          })
        }
      />

      <div className="p-4 rounded-lg bg-muted/50 border border-border/50">
        <h4 className="font-medium mb-3 text-sm">Detection</h4>
        <div className="flex gap-6 items-end">
          <div className="flex-1 min-w-0">
            <ModelSelect
              label="Model"
              value={config.detection.model}
              models={models.filter((m) => m.type === "detection")}
              error={config.enable}
              onChange={(model) =>
                setFaceConfig({
                  ...config,
                  detection: { ...config.detection, model },
                })
              }
            />
          </div>
          <div className="w-48 shrink-0">
            <Threshold
              label="Threshold"
              value={config.detection.threshold}
              onChange={(threshold) =>
                setFaceConfig({
                  ...config,
                  detection: { ...config.detection, threshold },
                })
              }
            />
          </div>
        </div>
        <h4 className="font-medium my-3 text-sm">Recognition</h4>
        <div className="flex gap-6 items-end">
          <div className="flex-1 min-w-0">
            <ModelSelect
              label="Model"
              value={config.recognition.model}
              models={models.filter((m) => m.type === "recognition")}
              error={config.enable}
              onChange={(model) =>
                setFaceConfig({
                  ...config,
                  recognition: { ...config.recognition, model },
                })
              }
            />
          </div>
          <div className="w-48 shrink-0">
            <Threshold
              label="Threshold"
              value={config.recognition.threshold}
              onChange={(threshold) =>
                setFaceConfig({
                  ...config,
                  recognition: { ...config.recognition, threshold },
                })
              }
            />
          </div>
        </div>
      </div>

      <div className="p-4 rounded-lg bg-muted/50 border border-border/50 space-y-3">
        <h4 className="font-medium text-sm">Anti-Spoofing</h4>
        <div className="grid gap-2">
          <Label
            htmlFor="anti-spoofing-method"
            className="text-xs text-muted-foreground"
          >
            AI Model
          </Label>
          <Select
            value={aiModelValue}
            onValueChange={(value) => {
              if (value === disabledOption) {
                setFaceConfig({
                  ...config,
                  anti_spoofing: { ...config.anti_spoofing, enable: false },
                });
                return;
              }

              setFaceConfig({
                ...config,
                anti_spoofing: {
                  ...config.anti_spoofing,
                  enable: true,
                  model: { ...config.anti_spoofing.model, path: value },
                },
              });
            }}
          >
            <SelectTrigger id="anti-spoofing-method" className="h-10 w-full">
              <SelectValue placeholder="Select AI anti-spoofing method" />
            </SelectTrigger>
            <SelectContent>
              <SelectItem value={disabledOption}>Disable</SelectItem>
              {aiModelValue === unavailableAiModelOption && (
                <SelectItem value={unavailableAiModelOption} disabled>
                  Selected anti-spoofing model unavailable
                </SelectItem>
              )}
              {antiSpoofModels.length > 0 ? (
                antiSpoofModels.map((model) => (
                  <SelectItem key={model.path} value={model.path}>
                    {model.path.split("/").pop()}
                  </SelectItem>
                ))
              ) : (
                <SelectItem value="__no_ai_models__" disabled>
                  No anti-spoofing models available
                </SelectItem>
              )}
            </SelectContent>
          </Select>
        </div>

        {config.anti_spoofing.enable && (
          <div className="w-48">
            <Threshold
              label="Threshold"
              value={config.anti_spoofing.model.threshold}
              onChange={(threshold) =>
                setFaceConfig({
                  ...config,
                  anti_spoofing: {
                    ...config.anti_spoofing,
                    model: { ...config.anti_spoofing.model, threshold },
                  },
                })
              }
            />
          </div>
        )}

        <div className="grid gap-2">
          <Label htmlFor="ir-device" className="text-xs text-muted-foreground">
            IR Camera
          </Label>
          <Select
            value={irCameraValue}
            onValueChange={(value) => {
              if (value === disabledOption) {
                setFaceConfig({
                  ...config,
                  anti_spoofing: { ...config.anti_spoofing, ir_camera: null },
                });
                return;
              }

              setFaceConfig({
                ...config,
                anti_spoofing: { ...config.anti_spoofing, ir_camera: value },
              });
            }}
          >
            <SelectTrigger id="ir-device" className="h-10 w-full">
              <SelectValue placeholder="Select IR Camera Device" />
            </SelectTrigger>
            <SelectContent>
              <SelectItem value={disabledOption}>Disable</SelectItem>
              {irCameraValue === unavailableIrDeviceOption && (
                <SelectItem value={unavailableIrDeviceOption} disabled>
                  Selected IR camera unavailable
                </SelectItem>
              )}
              {videoDevices.length > 0 ? (
                videoDevices.map((device) => (
                  <SelectItem key={device.path} value={device.path}>
                    {device.display_name}
                  </SelectItem>
                ))
              ) : (
                <SelectItem value="__no_ir_devices__" disabled>
                  No video devices found
                </SelectItem>
              )}
            </SelectContent>
          </Select>
        </div>
      </div>

    </div>
  );
}
