import { convertFileSrc, invoke } from "@tauri-apps/api/core";
import { Camera, Circle, Square, Trash2 } from "lucide-react";
import { useCallback, useEffect, useRef, useState } from "react";
import { toast } from "sonner";
import { Button } from "@/components/ui/button";

export function FaceCaptureSection() {
  const videoRef = useRef<HTMLVideoElement>(null);
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const [stream, setStream] = useState<MediaStream | null>(null);
  const [capturing, setCapturing] = useState(false);
  const [faceImages, setFaceImages] = useState<string[]>([]);

  const loadFaceImages = useCallback(async () => {
    try {
      const images = await invoke<string[]>("list_face_images");
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
      if (stream) {
        for (const track of stream.getTracks()) {
          track.stop();
        }
      }
    };
  }, [stream]);

  // Attach stream to video element when stream changes
  useEffect(() => {
    if (videoRef.current && stream) {
      videoRef.current.srcObject = stream;
      videoRef.current.play().catch(console.error);
    }
  }, [stream]);

  async function startCamera() {
    try {
      const mediaStream = await navigator.mediaDevices.getUserMedia({
        video: { width: 640, height: 480, facingMode: "user" },
      });
      setStream(mediaStream);
      setCapturing(true);
    } catch (err) {
      toast.error("Failed to access camera");
      console.error(err);
    }
  }

  function stopCamera() {
    if (stream) {
      for (const track of stream.getTracks()) {
        track.stop();
      }
      setStream(null);
    }
    setCapturing(false);
  }

  async function capturePhoto() {
    if (!videoRef.current || !canvasRef.current) return;

    const video = videoRef.current;
    const canvas = canvasRef.current;
    canvas.width = video.videoWidth;
    canvas.height = video.videoHeight;

    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    ctx.drawImage(video, 0, 0);
    const dataUrl = canvas.toDataURL("image/jpeg", 0.9);
    const base64Data = dataUrl.split(",")[1];

    try {
      await invoke<string>("save_face_image", {
        imageData: base64Data,
      });
      toast.success("Face image saved!");
      await loadFaceImages();
    } catch (err) {
      toast.error(`Failed to save face image: ${err}`);
    }
  }

  async function deleteFace(path: string) {
    try {
      await invoke("delete_face_image", { path });
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
        {/* Camera Preview */}
        <div className="relative aspect-video bg-black rounded-lg overflow-hidden">
          <video
            ref={videoRef}
            autoPlay
            playsInline
            muted
            className={`w-full h-full object-cover ${capturing ? "" : "hidden"}`}
          />
          {!capturing && (
            <div className="absolute inset-0 flex items-center justify-center text-muted-foreground">
              <Camera className="w-12 h-12 opacity-50" />
            </div>
          )}
          <canvas ref={canvasRef} className="hidden" />
        </div>

        {/* Controls */}
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

        {/* Saved Faces */}
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
