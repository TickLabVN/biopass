import { BaseDirectory, writeFile } from "@tauri-apps/plugin-fs";
import { Camera, CameraOff, Save } from "lucide-react";
import { useCallback, useEffect, useRef, useState } from "react";
import { toast } from "sonner";
import { Button } from "@/components/ui/button";

export function CameraSample() {
  const videoRef = useRef<HTMLVideoElement>(null);
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const [stream, setStream] = useState<MediaStream | null>(null);
  const [capturedImage, setCapturedImage] = useState<string | null>(null);
  const [isSaving, setIsSaving] = useState(false);

  useEffect(() => {
    if (stream && videoRef.current) {
      videoRef.current.srcObject = stream;
    }
  }, [stream]);

  const startCamera = async () => {
    try {
      const mediaStream = await navigator.mediaDevices.getUserMedia({
        video: { facingMode: "user" },
      });
      setStream(mediaStream);
    } catch (err) {
      console.error("Error accessing camera:", err);
      toast.error("Failed to access camera");
    }
  };

  const saveImage = async () => {
    if (!capturedImage) return;

    try {
      setIsSaving(true);
      // Extract base64 content
      const base64Content = capturedImage.split(",")[1];
      const byteCharacters = atob(base64Content);
      const byteNumbers = new Array(byteCharacters.length);
      for (let i = 0; i < byteCharacters.length; i++) {
        byteNumbers[i] = byteCharacters.charCodeAt(i);
      }
      const byteArray = new Uint8Array(byteNumbers);

      const fileName = `capture_${Date.now()}.png`;

      await writeFile(fileName, byteArray, {
        baseDir: BaseDirectory.Download,
      });

      toast.success(`Image saved to Downloads as ${fileName}`);
    } catch (err) {
      console.error("Error saving image:", err);
      toast.error("Failed to save image");
    } finally {
      setIsSaving(false);
    }
  };

  const stopCamera = useCallback(() => {
    if (stream) {
      for (const track of stream.getTracks()) {
        track.stop();
      }
      setStream(null);
    }
  }, [stream]);

  const capturePhoto = () => {
    if (videoRef.current && canvasRef.current) {
      const video = videoRef.current;
      const canvas = canvasRef.current;
      canvas.width = video.videoWidth;
      canvas.height = video.videoHeight;
      const context = canvas.getContext("2d");
      if (context) {
        context.drawImage(video, 0, 0, canvas.width, canvas.height);
        const imageData = canvas.toDataURL("image/png");
        setCapturedImage(imageData);
      }
    }
  };

  return (
    <div className="flex flex-col items-center gap-4 p-6 border rounded-xl bg-card shadow-sm">
      <div className="flex items-center gap-2 mb-2">
        <Camera className="w-5 h-5 text-primary" />
        <h2 className="text-xl font-semibold">Camera Input</h2>
      </div>

      <div className="relative w-full max-w-md aspect-video bg-black rounded-lg overflow-hidden border">
        {stream ? (
          <video
            ref={videoRef}
            autoPlay
            playsInline
            muted
            className="w-full h-full object-cover"
          />
        ) : (
          <div className="absolute inset-0 flex items-center justify-center text-muted-foreground">
            <CameraOff className="w-12 h-12 mb-2 opacity-20" />
            <p>Camera is off</p>
          </div>
        )}
      </div>

      <div className="flex gap-2">
        {!stream ? (
          <Button onClick={startCamera}>Start Camera</Button>
        ) : (
          <>
            <Button variant="secondary" onClick={stopCamera}>
              Stop Camera
            </Button>
            <Button onClick={capturePhoto}>Capture Photo</Button>
          </>
        )}
      </div>

      {capturedImage && (
        <div className="mt-4 flex flex-col items-center gap-2">
          <p className="text-sm font-medium">Captured Preview:</p>
          <div className="relative group">
            <img
              src={capturedImage}
              alt="Captured"
              className="w-48 rounded-md border shadow-md"
            />
            <div className="absolute top-2 right-2 flex gap-1 opacity-100 sm:opacity-0 sm:group-hover:opacity-100 transition-opacity">
              <Button
                size="icon"
                variant="secondary"
                className="w-8 h-8 rounded-full shadow-lg"
                onClick={saveImage}
                disabled={isSaving}
                title="Save to Downloads"
              >
                <Save
                  className={`w-4 h-4 ${isSaving ? "animate-pulse" : ""}`}
                />
              </Button>
              <Button
                size="icon"
                variant="destructive"
                className="w-8 h-8 rounded-full shadow-lg"
                onClick={() => setCapturedImage(null)}
                title="Discard"
              >
                <CameraOff className="w-4 h-4" />
              </Button>
            </div>
          </div>
        </div>
      )}

      <canvas ref={canvasRef} className="hidden" />
    </div>
  );
}
