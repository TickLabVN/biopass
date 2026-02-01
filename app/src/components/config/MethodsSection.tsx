import { convertFileSrc, invoke } from "@tauri-apps/api/core";
import {
  Camera,
  ChevronDown,
  Circle,
  Fingerprint,
  Mic,
  ScanFace,
  ShieldCheck,
  Square,
  Trash2,
} from "lucide-react";
import { useCallback, useEffect, useRef, useState } from "react";
import { toast } from "sonner";
import { Button } from "@/components/ui/button";
import { Slider } from "@/components/ui/slider";
import { Switch } from "@/components/ui/switch";
import type {
  FaceMethodConfig,
  FingerprintMethodConfig,
  MethodsConfig,
  VoiceMethodConfig,
} from "@/types/config";

interface Props {
  methods: MethodsConfig;
  onChange: (methods: MethodsConfig) => void;
}

export function MethodsSection({ methods, onChange }: Props) {
  const [expandedMethod, setExpandedMethod] = useState<string | null>("face");

  const updateFace = (face: FaceMethodConfig) => onChange({ ...methods, face });
  const updateVoice = (voice: VoiceMethodConfig) =>
    onChange({ ...methods, voice });
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
          <div className="grid gap-4 pt-4">
            {/* Face Capture */}
            <FaceCaptureSection />

            {/* Detection */}
            <div className="p-4 rounded-lg bg-muted/50 border border-border/50">
              <h4 className="font-medium mb-3">Detection</h4>
              <div className="grid gap-3">
                <InputField
                  label="Model Path"
                  value={methods.face.detection.model}
                  onChange={(model) =>
                    updateFace({
                      ...methods.face,
                      detection: { ...methods.face.detection, model },
                    })
                  }
                />
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

            {/* Recognition */}
            <div className="p-4 rounded-lg bg-muted/50 border border-border/50">
              <h4 className="font-medium mb-3">Recognition</h4>
              <div className="grid gap-3">
                <InputField
                  label="Model Path"
                  value={methods.face.recognition.model}
                  onChange={(model) =>
                    updateFace({
                      ...methods.face,
                      recognition: { ...methods.face.recognition, model },
                    })
                  }
                />
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

            {/* Anti-Spoofing */}
            <div className="p-4 rounded-lg bg-muted/50 border border-border/50">
              <div className="flex items-center justify-between mb-3">
                <h4 className="font-medium">Anti-Spoofing</h4>
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
                <div className="grid gap-3">
                  <InputField
                    label="Model Path"
                    value={methods.face.anti_spoofing.model}
                    onChange={(model) =>
                      updateFace({
                        ...methods.face,
                        anti_spoofing: { ...methods.face.anti_spoofing, model },
                      })
                    }
                  />
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
          onToggle={(enable) => updateFingerprint({ enable })}
          expanded={expandedMethod === "fingerprint"}
          onExpand={() =>
            setExpandedMethod(
              expandedMethod === "fingerprint" ? null : "fingerprint",
            )
          }
        >
          <div className="pt-4 text-sm text-muted-foreground">
            Fingerprint authentication uses the system's fingerprint reader via
            fprintd. No additional configuration required.
          </div>
        </MethodCard>

        {/* Voice Authentication */}
        <MethodCard
          title="Voice Recognition"
          icon={methodIcons.voice}
          color={methodColors.voice}
          enabled={methods.voice.enable}
          onToggle={(enable) => updateVoice({ ...methods.voice, enable })}
          expanded={expandedMethod === "voice"}
          onExpand={() =>
            setExpandedMethod(expandedMethod === "voice" ? null : "voice")
          }
        >
          <div className="grid gap-4 pt-4">
            {/* Voice Recording */}
            <VoiceRecordingSection />

            <div className="p-4 rounded-lg bg-muted/50 border border-border/50">
              <h4 className="font-medium mb-3">Recognition Settings</h4>
              <div className="grid gap-3">
                <InputField
                  label="Model Path"
                  value={methods.voice.model}
                  onChange={(model) => updateVoice({ ...methods.voice, model })}
                />
                <SliderField
                  label="Threshold"
                  value={methods.voice.threshold}
                  onChange={(threshold) =>
                    updateVoice({ ...methods.voice, threshold })
                  }
                />
              </div>
            </div>
          </div>
        </MethodCard>
      </div>
    </div>
  );
}

function FaceCaptureSection() {
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
                    className="absolute top-1 right-1 p-1 rounded bg-destructive/80 text-destructive-foreground opacity-0 group-hover:opacity-100 transition-opacity cursor-pointer"
                  >
                    <Trash2 className="w-3 h-3" />
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

function VoiceRecordingSection() {
  const mediaRecorderRef = useRef<MediaRecorder | null>(null);
  const chunksRef = useRef<Blob[]>([]);
  const [recording, setRecording] = useState(false);
  const [voiceRecordings, setVoiceRecordings] = useState<string[]>([]);

  const loadVoiceRecordings = useCallback(async () => {
    try {
      const recordings = await invoke<string[]>("list_voice_recordings");
      setVoiceRecordings(recordings);
    } catch (err) {
      console.error("Failed to load voice recordings:", err);
    }
  }, []);

  useEffect(() => {
    loadVoiceRecordings();
  }, [loadVoiceRecordings]);

  async function startRecording() {
    try {
      const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
      const mediaRecorder = new MediaRecorder(stream);
      mediaRecorderRef.current = mediaRecorder;
      chunksRef.current = [];

      mediaRecorder.ondataavailable = (e) => {
        if (e.data.size > 0) {
          chunksRef.current.push(e.data);
        }
      };

      mediaRecorder.onstop = async () => {
        const blob = new Blob(chunksRef.current, { type: "audio/webm" });
        for (const track of stream.getTracks()) {
          track.stop();
        }

        // Convert to base64
        const reader = new FileReader();
        reader.onloadend = async () => {
          const base64Data = (reader.result as string).split(",")[1];
          try {
            await invoke("save_voice_recording", {
              audioData: base64Data,
            });
            toast.success("Voice recording saved!");
            await loadVoiceRecordings();
          } catch (err) {
            toast.error(`Failed to save recording: ${err}`);
          }
        };
        reader.readAsDataURL(blob);
      };

      mediaRecorder.start();
      setRecording(true);
    } catch (err) {
      toast.error("Failed to access microphone");
      console.error(err);
    }
  }

  function stopRecording() {
    if (mediaRecorderRef.current && recording) {
      mediaRecorderRef.current.stop();
      setRecording(false);
    }
  }

  async function deleteVoice(path: string) {
    try {
      await invoke("delete_voice_recording", { path });
      toast.success("Voice recording deleted");
      await loadVoiceRecordings();
    } catch (err) {
      toast.error(`Failed to delete: ${err}`);
    }
  }

  return (
    <div className="p-4 rounded-lg bg-muted/50 border border-border/50">
      <h4 className="font-medium mb-3 flex items-center gap-2">
        <Mic className="w-4 h-4" />
        Voice Recording
      </h4>

      <div className="grid gap-4">
        {/* Recording Controls */}
        <div className="flex items-center gap-4">
          {!recording ? (
            <Button onClick={startRecording} className="flex-1">
              <Circle className="w-4 h-4 mr-2 text-red-500" />
              Start Recording
            </Button>
          ) : (
            <Button
              variant="destructive"
              onClick={stopRecording}
              className="flex-1"
            >
              <Square className="w-4 h-4 mr-2" />
              Stop Recording
            </Button>
          )}
          {recording && (
            <div className="flex items-center gap-2 text-red-500">
              <div className="w-3 h-3 rounded-full bg-red-500 animate-pulse" />
              <span className="text-sm font-medium">Recording...</span>
            </div>
          )}
        </div>

        {/* Saved Recordings */}
        {voiceRecordings.length > 0 && (
          <div>
            <p className="text-sm text-muted-foreground mb-2">
              Saved Recordings ({voiceRecordings.length})
            </p>
            <div className="grid gap-2">
              {voiceRecordings.map((path) => (
                <div
                  key={path}
                  className="flex items-center justify-between p-2 bg-background rounded-lg border"
                >
                  <span className="text-sm truncate flex-1">
                    {path.split("/").pop()}
                  </span>
                  <button
                    type="button"
                    onClick={() => deleteVoice(path)}
                    className="p-1 rounded hover:bg-destructive/20 text-destructive cursor-pointer"
                  >
                    <Trash2 className="w-4 h-4" />
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

function MethodCard({
  title,
  icon,
  color,
  enabled,
  onToggle,
  expanded,
  onExpand,
  children,
}: {
  title: string;
  icon: React.ReactNode;
  color: string;
  enabled: boolean;
  onToggle: (enabled: boolean) => void;
  expanded: boolean;
  onExpand: () => void;
  children: React.ReactNode;
}) {
  return (
    <div
      className={`rounded-lg border transition-all ${enabled ? "border-border" : "border-border/50 opacity-60"}`}
    >
      <div className="flex items-center justify-between p-4">
        <button
          type="button"
          onClick={onExpand}
          className="flex items-center gap-3 flex-1 text-left cursor-pointer"
        >
          <span
            className={`w-10 h-10 rounded-lg bg-linear-to-br ${color} flex items-center justify-center text-xl`}
          >
            {icon}
          </span>
          <div>
            <h3 className="font-medium">{title}</h3>
            <p className="text-xs text-muted-foreground">
              {enabled ? "Enabled" : "Disabled"}
            </p>
          </div>
          <ChevronDown
            className={`ml-auto mr-4 w-4 h-4 transition-transform ${expanded ? "rotate-180" : ""}`}
          />
        </button>
        <Switch
          checked={enabled}
          onCheckedChange={onToggle}
          className="cursor-pointer"
        />
      </div>
      {expanded && enabled && (
        <div className="px-4 pb-4 border-t border-border/50">{children}</div>
      )}
    </div>
  );
}

function InputField({
  label,
  value,
  onChange,
}: {
  label: string;
  value: string;
  onChange: (value: string) => void;
}) {
  const id = `input-${label.toLowerCase().replace(/\s+/g, "-")}`;
  return (
    <div className="grid gap-1">
      <label htmlFor={id} className="text-xs font-medium text-muted-foreground">
        {label}
      </label>
      <input
        id={id}
        type="text"
        value={value}
        onChange={(e) => onChange(e.target.value)}
        className="w-full px-3 py-2 text-sm rounded-lg border border-border bg-background focus:outline-none focus:ring-2 focus:ring-primary/50"
      />
    </div>
  );
}

function SliderField({
  label,
  value,
  onChange,
}: {
  label: string;
  value: number;
  onChange: (value: number) => void;
}) {
  return (
    <div className="grid gap-2">
      <div className="flex justify-between">
        <span className="text-xs font-medium text-muted-foreground">
          {label}
        </span>
        <span className="text-xs font-mono text-muted-foreground">
          {value.toFixed(2)}
        </span>
      </div>
      <Slider
        value={[value]}
        min={0}
        max={1}
        step={0.01}
        onValueChange={(values) => onChange(values[0])}
      />
    </div>
  );
}
