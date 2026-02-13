import { invoke } from "@tauri-apps/api/core";
import { Circle, Mic, Square, Trash2 } from "lucide-react";
import { useCallback, useEffect, useRef, useState } from "react";
import { toast } from "sonner";
import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";

export function VoiceRecordingSection() {
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
