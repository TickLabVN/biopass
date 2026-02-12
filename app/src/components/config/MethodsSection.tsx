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
import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";
import { Label } from "@/components/ui/label";
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select";
import { Slider } from "@/components/ui/slider";
import { Switch } from "@/components/ui/switch";
import { cn } from "@/lib/utils";
import type {
  FaceMethodConfig,
  FingerprintMethodConfig,
  MethodsConfig,
  ModelConfig,
  VoiceMethodConfig,
} from "@/types/config";
import { ModelStatus } from "../models/ModelStatus";

interface Props {
  methods: MethodsConfig;
  models: ModelConfig[];
  onChange: (methods: MethodsConfig) => void;
}

export function MethodsSection({ methods, models, onChange }: Props) {
  const [expandedMethod, setExpandedMethod] = useState<string | null>("face");
  const [videoDevices, setVideoDevices] = useState<string[]>([]);
  const [fingerprintDevices, setFingerprintDevices] = useState<
    { name: string; driver: string; device_id: string }[]
  >([]);

  useEffect(() => {
    const fetchDevices = async () => {
      try {
        const [vDevices, fDevices] = await Promise.all([
          invoke<string[]>("list_video_devices"),
          invoke<{ name: string; driver: string; device_id: string }[]>(
            "list_fingerprint_devices",
          ),
        ]);
        setVideoDevices(vDevices);
        setFingerprintDevices(fDevices);
      } catch (err) {
        console.error("Failed to fetch devices:", err);
      }
    };
    fetchDevices();
  }, []);

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
            <div className="p-4 rounded-lg bg-muted/50 border border-border/50">
              <div className="flex items-center justify-between mb-3">
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
            <div className="p-4 rounded-lg bg-muted/50 border border-border/50">
              <div className="flex items-center justify-between mb-3">
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
          <div className="pt-4 overflow-hidden">
            <FingerprintSection
              config={methods.fingerprint}
              devices={fingerprintDevices}
              onUpdate={(fingerprint) => updateFingerprint(fingerprint)}
            />
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
              <h4 className="font-medium mb-3 text-sm">Recognition Settings</h4>
              <div className="flex gap-6 items-end">
                <div className="flex-1 min-w-0">
                  <ModelSelectField
                    label="Model"
                    value={methods.voice.model}
                    models={models.filter((m) => m.type === "voice")}
                    error={methods.voice.enable}
                    onChange={(model) =>
                      updateVoice({ ...methods.voice, model })
                    }
                  />
                </div>
                <div className="w-48 shrink-0">
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

function VoiceRecordingSection() {
  const [recording, setRecording] = useState(false);
  const [voiceRecordings, setVoiceRecordings] = useState<string[]>([]);
  const audioContextRef = useRef<AudioContext | null>(null);
  const inputRef = useRef<MediaStreamAudioSourceNode | null>(null);
  const streamRef = useRef<MediaStream | null>(null);
  const audioDataRef = useRef<Float32Array[]>([]);

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

  // WAV encoding helper
  const encodeWAV = (samples: Float32Array, sampleRate: number) => {
    const buffer = new ArrayBuffer(44 + samples.length * 2);
    const view = new DataView(buffer);

    /* RIFF identifier */
    writeString(view, 0, "RIFF");
    /* RIFF chunk length */
    view.setUint32(4, 36 + samples.length * 2, true);
    /* RIFF type */
    writeString(view, 8, "WAVE");
    /* format chunk identifier */
    writeString(view, 12, "fmt ");
    /* format chunk length */
    view.setUint32(16, 16, true);
    /* sample format (raw) */
    view.setUint16(20, 1, true);
    /* channel count */
    view.setUint16(22, 1, true);
    /* sample rate */
    view.setUint32(24, sampleRate, true);
    /* byte rate (sample rate * block align) */
    view.setUint32(28, sampleRate * 2, true);
    /* block align (channel count * bytes per sample) */
    view.setUint16(32, 2, true);
    /* bits per sample */
    view.setUint16(34, 16, true);
    /* data chunk identifier */
    writeString(view, 36, "data");
    /* data chunk length */
    view.setUint32(40, samples.length * 2, true);

    // Write samples
    let offset = 44;
    for (let i = 0; i < samples.length; i++, offset += 2) {
      const s = Math.max(-1, Math.min(1, samples[i]));
      view.setInt16(offset, s < 0 ? s * 0x8000 : s * 0x7fff, true);
    }

    return new Blob([view], { type: "audio/wav" });
  };

  const writeString = (view: DataView, offset: number, string: string) => {
    for (let i = 0; i < string.length; i++) {
      view.setUint8(offset + i, string.charCodeAt(i));
    }
  };

  async function startRecording() {
    try {
      const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
      streamRef.current = stream;

      const audioContext = new AudioContext();
      audioContextRef.current = audioContext;

      // Define the worklet code as a string
      const workletCode = `
        class VoiceProcessor extends AudioWorkletProcessor {
          process(inputs, outputs, parameters) {
            const input = inputs[0];
            if (input.length > 0) {
              const channelData = input[0];
              this.port.postMessage(channelData);
            }
            return true;
          }
        }
        registerProcessor('voice-processor', VoiceProcessor);
      `;

      const blob = new Blob([workletCode], { type: "application/javascript" });
      const url = URL.createObjectURL(blob);
      await audioContext.audioWorklet.addModule(url);

      const source = audioContext.createMediaStreamSource(stream);
      inputRef.current = source;

      const workletNode = new AudioWorkletNode(audioContext, "voice-processor");

      audioDataRef.current = [];
      workletNode.port.onmessage = (e) => {
        audioDataRef.current.push(new Float32Array(e.data));
      };

      source.connect(workletNode);
      workletNode.connect(audioContext.destination);

      setRecording(true);
    } catch (err) {
      toast.error("Failed to access microphone");
      console.error(err);
    }
  }

  async function stopRecording() {
    if (!recording) return;

    // Stop capturing
    if (audioContextRef.current) {
      if (inputRef.current) {
        inputRef.current.disconnect();
      }
      // Releasing resources
      audioContextRef.current.close().catch(console.error);
    }

    if (streamRef.current) {
      for (const track of streamRef.current.getTracks()) {
        track.stop();
      }
    }

    const sampleRate = audioContextRef.current?.sampleRate || 44100;

    setRecording(false);

    // Flatten data
    const totalLength = audioDataRef.current.reduce(
      (acc, val) => acc + val.length,
      0,
    );
    const result = new Float32Array(totalLength);
    let offset = 0;
    for (const buffer of audioDataRef.current) {
      result.set(buffer, offset);
      offset += buffer.length;
    }

    // Encode to WAV
    const wavBlob = encodeWAV(result, sampleRate);

    // Convert to base64
    const reader = new FileReader();
    reader.onloadend = async () => {
      const base64Data = (reader.result as string).split(",")[1];
      try {
        await invoke("save_voice_recording", {
          audioData: base64Data,
        });
        toast.success("Voice recording saved as WAV!");
        await loadVoiceRecordings();
      } catch (err) {
        toast.error(`Failed to save recording: ${err}`);
      }
    };
    reader.readAsDataURL(wavBlob);
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
            <Badge
              variant="destructive"
              className="animate-pulse gap-1.5 h-7 px-3"
            >
              <div className="w-2 h-2 rounded-full bg-white animate-pulse" />
              Recording...
            </Badge>
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

function FingerprintSection({
  config,
  devices,
  onUpdate,
}: {
  config: FingerprintMethodConfig;
  devices: { name: string; driver: string; device_id: string }[];
  onUpdate: (config: FingerprintMethodConfig) => void;
}) {
  const [selectedDevice, setSelectedDevice] = useState<string>("");
  const [selectedFinger, setSelectedFinger] =
    useState<string>("right-index-finger");
  const [isAdding, setIsAdding] = useState(false);
  const [username, setUsername] = useState<string>("");

  useEffect(() => {
    const fetchUsername = async () => {
      try {
        const user = await invoke<string>("get_current_username");
        setUsername(user);

        // Sync enrolled fingers from backend
        const enrolledFingers = await invoke<string[]>(
          "list_enrolled_fingerprints",
          { username: user },
        );

        // Update local config if there's a mismatch (best effort)
        const currentFingerNames = config.fingers.map((f) => f.name);
        const needsSync =
          enrolledFingers.some((f) => !currentFingerNames.includes(f)) ||
          currentFingerNames.some((f) => !enrolledFingers.includes(f));

        if (needsSync) {
          const syncedFingers = enrolledFingers.map((name) => {
            const existing = config.fingers.find((f) => f.name === name);
            return (
              existing || { name, created_at: Math.floor(Date.now() / 1000) }
            );
          });
          onUpdate({ ...config, fingers: syncedFingers });
        }
      } catch (err) {
        console.error("Failed to sync fingerprints:", err);
      }
    };
    fetchUsername();
  }, [config, onUpdate]);

  const fingerOptions = [
    "left-thumb",
    "left-index-finger",
    "left-middle-finger",
    "left-ring-finger",
    "left-little-finger",
    "right-thumb",
    "right-index-finger",
    "right-middle-finger",
    "right-ring-finger",
    "right-little-finger",
  ];

  const handleAdd = async () => {
    if (!selectedDevice) {
      toast.error("Please select a fingerprint device");
      return;
    }

    setIsAdding(true);
    const toastId = toast.loading(
      `Enrolling ${selectedFinger.replace(/-/g, " ")}... Please touch the sensor.`,
    );

    try {
      await invoke("enroll_fingerprint", {
        username: username,
        fingerName: selectedFinger,
      });

      toast.success(`${selectedFinger.replace(/-/g, " ")} enrolled!`, {
        id: toastId,
      });

      // The backend saves to config, but we update UI immediately
      onUpdate({
        ...config,
        fingers: [
          ...config.fingers,
          { name: selectedFinger, created_at: Math.floor(Date.now() / 1000) },
        ],
      });
    } catch (err) {
      toast.error(`Enrollment failed: ${err}`, { id: toastId });
    } finally {
      setIsAdding(false);
    }
  };

  const handleDelete = async (fingerName: string) => {
    try {
      await invoke("remove_fingerprint", {
        username: username,
        fingerName: fingerName,
      });
      toast.success(`${fingerName.replace(/-/g, " ")} deleted`);

      onUpdate({
        ...config,
        fingers: config.fingers.filter((f) => f.name !== fingerName),
      });
    } catch (err) {
      toast.error(`Delete failed: ${err}`);
    }
  };

  return (
    <div className="grid gap-4">
      <div className="p-4 rounded-lg bg-muted/50 border border-border/50">
        <h4 className="font-medium mb-3 text-sm">Registered Fingers</h4>
        {config.fingers.length > 0 ? (
          <div className="grid gap-2">
            {config.fingers.map((f) => (
              <div
                key={f.name}
                className="flex items-center justify-between p-2 bg-background rounded-lg border"
              >
                <div className="flex flex-col">
                  <span className="text-sm font-medium capitalize">
                    {f.name.replace(/-/g, " ")}
                  </span>
                  <span className="text-[10px] text-muted-foreground italic">
                    Added on{" "}
                    {new Date(f.created_at * 1000).toLocaleDateString()}
                  </span>
                </div>
                <button
                  type="button"
                  onClick={() => handleDelete(f.name)}
                  className="p-1 rounded hover:bg-destructive/20 text-destructive cursor-pointer transition-colors"
                >
                  <Trash2 className="w-4 h-4" />
                </button>
              </div>
            ))}
          </div>
        ) : (
          <p className="text-xs text-muted-foreground italic">
            No fingers registered yet.
          </p>
        )}
      </div>

      <div className="p-4 rounded-lg bg-muted/50 border border-border/50">
        <h4 className="font-medium mb-3 text-sm">Enroll New Finger</h4>
        <div className="grid gap-3">
          <div className="grid gap-2">
            <Label className="text-xs text-muted-foreground">
              Select Device
            </Label>
            <Select value={selectedDevice} onValueChange={setSelectedDevice}>
              <SelectTrigger className="h-9">
                <SelectValue placeholder="Select Device" />
              </SelectTrigger>
              <SelectContent>
                {devices.map((d) => (
                  <SelectItem key={d.device_id} value={d.device_id}>
                    {d.name} ({d.driver})
                  </SelectItem>
                ))}
                {devices.length === 0 && (
                  <SelectItem value="none" disabled>
                    No fingerprint readers found
                  </SelectItem>
                )}
              </SelectContent>
            </Select>
          </div>

          <div className="grid gap-2">
            <Label className="text-xs text-muted-foreground">
              Select Finger
            </Label>
            <Select value={selectedFinger} onValueChange={setSelectedFinger}>
              <SelectTrigger className="h-9">
                <SelectValue placeholder="Select Finger" />
              </SelectTrigger>
              <SelectContent>
                {fingerOptions.map((f) => (
                  <SelectItem
                    key={f}
                    value={f}
                    className="capitalize"
                    disabled={config.fingers.some((cf) => cf.name === f)}
                  >
                    {f.replace(/-/g, " ")}
                  </SelectItem>
                ))}
              </SelectContent>
            </Select>
          </div>

          <Button
            onClick={handleAdd}
            disabled={isAdding || !selectedDevice}
            className="w-full h-9 mt-1"
          >
            {isAdding ? "Enrolling..." : "Enroll Finger"}
          </Button>
        </div>
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
      className={cn(
        "group relative overflow-hidden rounded-xl border transition-all duration-300",
        expanded
          ? "border-border bg-muted/30 shadow-md ring-1 ring-border/50"
          : "border-border/30 bg-background/50 hover:bg-muted/20",
      )}
    >
      <div className="p-4 flex items-center justify-between gap-4">
        <div className="flex items-center gap-4 flex-1">
          <div
            className={cn(
              "w-10 h-10 rounded-lg bg-linear-to-br flex items-center justify-center transition-transform group-hover:scale-110 shadow-sm",
              color,
            )}
          >
            {icon}
          </div>
          <div className="flex-1">
            <h3 className="font-medium text-sm sm:text-base">{title}</h3>
            {!expanded && (
              <Badge
                variant={enabled ? "default" : "secondary"}
                className={cn(
                  "mt-1 text-[10px] h-4 px-1.5 transition-colors",
                  enabled
                    ? "bg-emerald-500/10 text-emerald-500 border-emerald-500/20"
                    : "bg-muted text-muted-foreground",
                )}
              >
                {enabled ? "Enabled" : "Disabled"}
              </Badge>
            )}
          </div>
        </div>

        <div className="flex items-center gap-4">
          <div className="flex items-center gap-2 pr-2 border-r border-border/50">
            <Switch
              checked={enabled}
              onCheckedChange={onToggle}
              className="cursor-pointer"
            />
          </div>
          <button
            type="button"
            onClick={onExpand}
            className={cn(
              "w-8 h-8 rounded-lg flex items-center justify-center hover:bg-muted/80 transition-all cursor-pointer",
              expanded && "bg-muted/80 rotate-180",
            )}
          >
            <ChevronDown className="w-4 h-4" />
          </button>
        </div>
      </div>

      <div
        className={cn(
          "grid transition-all duration-300 ease-in-out px-4 pb-4",
          expanded
            ? "grid-rows-[1fr] opacity-100"
            : "grid-rows-[0fr] opacity-0",
        )}
      >
        <div className="overflow-hidden">{children}</div>
      </div>
    </div>
  );
}

function ModelSelectField({
  label,
  value,
  models,
  error,
  onChange,
}: {
  label: string;
  value: string;
  models: ModelConfig[];
  error: boolean;
  onChange: (value: string) => void;
}) {
  const selectedModel = models.find((m) => m.path === value);
  const [statusMap, setStatusMap] = useState<Record<string, boolean>>({});

  useEffect(() => {
    const checkAllModels = async () => {
      const newStatusMap: Record<string, boolean> = {};
      await Promise.all(
        models.map(async (m) => {
          try {
            newStatusMap[m.path] = await invoke<boolean>("check_file_exists", {
              path: m.path,
            });
          } catch {
            newStatusMap[m.path] = false;
          }
        }),
      );
      setStatusMap(newStatusMap);
    };

    checkAllModels();
  }, [models]);

  const isValid = selectedModel && statusMap[selectedModel.path];

  return (
    <div className="grid gap-2">
      <div className="flex items-center justify-between">
        <Label className="text-xs text-muted-foreground">{label}</Label>
        <ModelStatus
          status={selectedModel ? statusMap[selectedModel.path] : "checking"}
          className="text-[10px]"
          size="sm"
        />
      </div>
      <Select value={value} onValueChange={onChange}>
        <SelectTrigger
          className={cn(
            "h-9 transition-colors text-sm",
            error && !isValid && "border-destructive ring-destructive/20",
          )}
        >
          <SelectValue placeholder="Select a model" />
        </SelectTrigger>
        <SelectContent>
          {models.length > 0 ? (
            models.map((model) => (
              <SelectItem key={model.path} value={model.path}>
                <div className="flex items-center justify-between w-[200px]">
                  <span className="truncate">
                    {model.name || model.path.split("/").pop()}
                  </span>
                  <ModelStatus
                    status={statusMap[model.path]}
                    size="sm"
                    className="ml-2 h-4"
                  />
                </div>
              </SelectItem>
            ))
          ) : (
            <SelectItem value="none" disabled>
              No models available
            </SelectItem>
          )}
        </SelectContent>
      </Select>
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
      <div className="flex items-center justify-between">
        <Label className="text-xs text-muted-foreground">{label}</Label>
        <span className="text-xs font-mono font-medium">
          {(value * 100).toFixed(0)}%
        </span>
      </div>
      <div className="h-9 flex items-center">
        <Slider
          value={[value]}
          max={1}
          step={0.01}
          onValueChange={([v]) => onChange(v)}
          className="cursor-pointer"
        />
      </div>
    </div>
  );
}
