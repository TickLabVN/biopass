import { convertFileSrc } from "@tauri-apps/api/core";
import { Camera, Circle, Square, Trash2, AlertCircle } from "lucide-react";
import { useCallback, useEffect, useRef, useState } from "react";
import { toast } from "sonner";
import { cmd } from "@/commands";
import { Button } from "@/components/ui/button";
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select";

const FPS_OPTIONS = [
  { label: "3 FPS", interval: 300, fps: 3 },
  { label: "15 FPS", interval: 66, fps: 15 },
  { label: "30 FPS", interval: 33, fps: 30 },
] as const;

function fpsToIndex(fps: number): number {
  const idx = FPS_OPTIONS.findIndex((o) => o.fps === fps);
  return idx >= 0 ? idx : 0;
}

export function FaceCapture({ selectedDevicePath, previewFps, onPreviewFpsChange }: {
  selectedDevicePath?: string;
  previewFps?: number;
  onPreviewFpsChange?: (fps: number) => void;
}) {
  const nativeImgRef = useRef<HTMLImageElement>(null);
  const pollingRef = useRef<number | null>(null);
  const [capturing, setCapturing] = useState(false);
  const [faceImages, setFaceImages] = useState<string[]>([]);
  const [nativeFrame, setNativeFrame] = useState<string | null>(null);
  const [fpsIndex, setFpsIndex] = useState(fpsToIndex(previewFps ?? 3));
  const capturedFrameRef = useRef<string | null>(null);

  const loadFaceImages = useCallback(async () => {
    try {
      const images = await cmd.face.listImages();
      setFaceImages(images);
    } catch (err) {
      console.error("Failed to load face images:", err);
    }
  }, []);

  useEffect(() => {
    loadFaceImages();
  }, [loadFaceImages]);

  useEffect(() => {
    return () => {
      stopPolling();
      cmd.face.stopCameraPreview().catch(() => {});
    };
  }, []);

  function stopPolling() {
    if (pollingRef.current !== null) {
      clearInterval(pollingRef.current);
      pollingRef.current = null;
    }
  }

  async function pollFrames() {
    stopPolling();
    const interval = FPS_OPTIONS[fpsIndex].interval;
    pollingRef.current = window.setInterval(async () => {
      try {
        const frame = await cmd.face.getPreviewFrame();
        if (frame) {
          capturedFrameRef.current = frame;
          setNativeFrame(`data:image/jpeg;base64,${frame}`);
        }
      } catch {
        // frame read failed, will retry
      }
    }, interval);
  }

  async function startCamera() {
    try {
      let device = selectedDevicePath;
      if (!device) {
        const devices = await cmd.face.listVideoDevices();
        const target = devices.find(
          (d) => !d.name.toLowerCase().includes("metadata"),
        );
        device = target?.path ?? "/dev/video0";
      }
      await cmd.face.startCameraPreview(device);
      pollFrames();
      setCapturing(true);
    } catch (err) {
      toast.error("Failed to access camera");
      console.error(err);
    }
  }

  function stopCamera() {
    stopPolling();
    setCapturing(false);
    setNativeFrame(null);
    capturedFrameRef.current = null;
    cmd.face.stopCameraPreview().catch(() => {});
  }

  async function capturePhoto() {
    const frame = capturedFrameRef.current;
    if (!frame) {
      toast.error("No frame available yet, please wait...");
      return;
    }
    await saveFaceImage(frame);
  }

  async function saveFaceImage(base64Data: string) {
    try {
      await cmd.face.saveImage(base64Data);
      toast.success("Face image saved!");
      await loadFaceImages();
    } catch (err) {
      toast.error(`Failed to save face image: ${err}`);
    }
  }

  async function deleteFace(path: string) {
    try {
      await cmd.face.deleteImage(path);
      toast.success("Face image deleted");
      await loadFaceImages();
    } catch (err) {
      toast.error(`Failed to delete: ${err}`);
    }
  }

  return (
    <div className="p-4 rounded-lg bg-muted/50 border border-border/50">
      <h4 className="font-medium mb-3 flex items-center gap-2">
        <Camera className="w-4 h-4" />
        Face Capture
      </h4>

      <div className="grid gap-4">
        <div className="relative aspect-video bg-black rounded-lg overflow-hidden">
          {nativeFrame && capturing ? (
            <img
              ref={nativeImgRef}
              src={nativeFrame}
              alt="Camera preview"
              className="w-full h-full object-cover"
            />
          ) : !capturing ? (
            <div className="absolute inset-0 flex items-center justify-center text-muted-foreground">
              <Camera className="w-12 h-12 opacity-50" />
            </div>
          ) : (
            <div className="absolute inset-0 flex items-center justify-center text-muted-foreground">
              <div className="text-center">
                <div className="animate-spin rounded-full h-6 w-6 border-b-2 border-primary mx-auto mb-2" />
                <p className="text-xs">Starting camera...</p>
              </div>
            </div>
          )}
        </div>

        <div className="flex items-center gap-2 text-xs text-amber-500 bg-amber-500/10 p-2 rounded">
          <AlertCircle className="w-3 h-3 shrink-0" />
          <span className="flex-1">
            Using native camera capture —{" "}
            {capturing
              ? `${FPS_OPTIONS[fpsIndex].label}`
              : "preview not started"}
          </span>
          <Select
            value={String(fpsIndex)}
            onValueChange={(v) => {
              const idx = Number(v);
              setFpsIndex(idx);
              if (onPreviewFpsChange) {
                onPreviewFpsChange(FPS_OPTIONS[idx].fps);
              }
              if (capturing) pollFrames();
            }}
          >
            <SelectTrigger className="h-6 text-xs w-20 border-amber-500/30">
              <SelectValue />
            </SelectTrigger>
            <SelectContent>
              {FPS_OPTIONS.map((opt, i) => (
                <SelectItem key={i} value={String(i)}>
                  {opt.label}
                </SelectItem>
              ))}
            </SelectContent>
          </Select>
        </div>

        <div className="flex gap-2">
          {!capturing ? (
            <Button onClick={startCamera} className="flex-1">
              <Camera className="w-4 h-4 mr-2" />
              Start Camera
            </Button>
          ) : (
            <>
              <Button onClick={capturePhoto} className="flex-1">
                <Circle className="w-4 h-4 mr-2" />
                Capture
              </Button>
              <Button variant="outline" onClick={stopCamera}>
                <Square className="w-4 h-4 mr-2" />
                Stop
              </Button>
            </>
          )}
        </div>

        {faceImages.length > 0 && (
          <div>
            <p className="text-sm text-muted-foreground mb-2">
              Saved Faces ({faceImages.length})
            </p>
            <div className="grid grid-cols-4 gap-2">
              {faceImages.map((path) => (
                <div key={path} className="relative group">
                  <div className="aspect-square bg-muted rounded-lg overflow-hidden">
                    <img
                      src={convertFileSrc(path)}
                      alt="Captured face"
                      className="w-full h-full object-cover"
                    />
                  </div>
                  <button
                    type="button"
                    onClick={() => deleteFace(path)}
                    className="absolute top-1 right-1 p-1 rounded bg-destructive/80 text-destructive-foreground cursor-pointer"
                  >
                    <Trash2 className="w-3 h-3 text-white" />
                  </button>
                </div>
              ))}
            </div>
          </div>
        )}
      </div>
    </div>
  );
}
