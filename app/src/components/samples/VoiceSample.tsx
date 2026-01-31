import { BaseDirectory, writeFile } from "@tauri-apps/plugin-fs";
import { Mic, MicOff, Play, Save, Square, Volume2 } from "lucide-react";
import { useCallback, useEffect, useRef, useState } from "react";
import { toast } from "sonner";

import { Button } from "@/components/ui/button";

export function VoiceSample() {
  const [isRecording, setIsRecording] = useState(false);
  const [isCapturing, setIsCapturing] = useState(false);
  const [audioLevels, setAudioLevels] = useState<number[]>(
    new Array(12).fill(0),
  );
  const [lastRecording, setLastRecording] = useState<Blob | null>(null);
  const [isSaving, setIsSaving] = useState(false);

  const audioContextRef = useRef<AudioContext | null>(null);
  const analyzerRef = useRef<AnalyserNode | null>(null);
  const streamRef = useRef<MediaStream | null>(null);
  const animationFrameRef = useRef<number | null>(null);
  const mediaRecorderRef = useRef<MediaRecorder | null>(null);
  const chunksRef = useRef<Blob[]>([]);

  const startRecording = async () => {
    try {
      const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
      streamRef.current = stream;

      const audioContext = new (
        window.AudioContext ||
        // biome-ignore lint/suspicious/noExplicitAny: webkitAudioContext is a legacy API
        (window as any).webkitAudioContext
      )();
      audioContextRef.current = audioContext;

      const analyzer = audioContext.createAnalyser();
      analyzer.fftSize = 256;
      analyzer.smoothingTimeConstant = 0.8;
      analyzerRef.current = analyzer;

      const source = audioContext.createMediaStreamSource(stream);
      source.connect(analyzer);

      if (audioContext.state === "suspended") {
        await audioContext.resume();
      }

      setIsRecording(true);
      updateAudioLevel();
      console.log("Microphone started successfully");
    } catch (err) {
      console.error("Error accessing microphone:", err);
      toast.error("Failed to access microphone");
    }
  };

  const startCapturing = useCallback(() => {
    if (!streamRef.current) return;

    chunksRef.current = [];
    const mediaRecorder = new MediaRecorder(streamRef.current);
    mediaRecorderRef.current = mediaRecorder;

    mediaRecorder.ondataavailable = (e) => {
      if (e.data.size > 0) {
        chunksRef.current.push(e.data);
      }
    };

    mediaRecorder.onstop = () => {
      const blob = new Blob(chunksRef.current, { type: "audio/webm" });
      setLastRecording(blob);
    };

    mediaRecorder.start();
    setIsCapturing(true);
    toast.info("Recording started...");
  }, []);

  const stopCapturing = useCallback(() => {
    if (mediaRecorderRef.current && isCapturing) {
      mediaRecorderRef.current.stop();
      setIsCapturing(false);
      toast.info("Recording stopped");
    }
  }, [isCapturing]);

  const saveAudio = async () => {
    if (!lastRecording) return;

    try {
      setIsSaving(true);
      const arrayBuffer = await lastRecording.arrayBuffer();
      const byteArray = new Uint8Array(arrayBuffer);
      const fileName = `voice_${Date.now()}.webm`;

      await writeFile(fileName, byteArray, {
        baseDir: BaseDirectory.Download,
      });

      toast.success(`Audio saved to Downloads as ${fileName} `);
    } catch (err) {
      console.error("Error saving audio:", err);
      toast.error("Failed to save audio");
    } finally {
      setIsSaving(false);
    }
  };

  const updateAudioLevel = () => {
    if (analyzerRef.current) {
      const dataArray = new Uint8Array(analyzerRef.current.frequencyBinCount);
      analyzerRef.current.getByteFrequencyData(dataArray);

      // Map 128 frequency bins to 12 bars
      const newLevels = [];
      const step = Math.floor(dataArray.length / 12);
      for (let i = 0; i < 12; i++) {
        let sum = 0;
        for (let j = 0; j < step; j++) {
          sum += dataArray[i * step + j];
        }
        newLevels.push(sum / step);
      }
      setAudioLevels(newLevels);

      animationFrameRef.current = requestAnimationFrame(updateAudioLevel);
    }
  };

  const stopRecording = useCallback(() => {
    if (animationFrameRef.current) {
      cancelAnimationFrame(animationFrameRef.current);
    }
    if (streamRef.current) {
      for (const track of streamRef.current.getTracks()) {
        track.stop();
      }
    }
    if (audioContextRef.current) {
      audioContextRef.current.close();
    }
    setIsRecording(false);
    setAudioLevels(new Array(12).fill(0));
  }, []);

  useEffect(() => {
    return () => stopRecording();
  }, [stopRecording]);

  return (
    <div className="flex flex-col items-center gap-4 p-6 border rounded-xl bg-card shadow-sm">
      <div className="flex items-center gap-2 mb-2">
        <Mic className="w-5 h-5 text-primary" />
        <h2 className="text-xl font-semibold">Voice Input</h2>
      </div>

      <div className="relative w-full max-w-md h-32 bg-secondary/20 rounded-lg overflow-hidden flex items-center justify-center border">
        {isRecording ? (
          <div className="flex items-end gap-1 h-16 w-full px-8 justify-center">
            {audioLevels.map((level, i) => (
              <div
                // biome-ignore lint/suspicious/noArrayIndexKey: level bars are fixed-order and decorative
                key={`level - ${i} `}
                className="w-2 bg-primary rounded-full transition-all duration-100 ease-out"
                style={{
                  height: `${Math.max(10, Math.min(100, (level / 255) * 100))}% `,
                  opacity: 0.5 + (level / 255) * 0.5,
                }}
              />
            ))}
          </div>
        ) : (
          <div className="flex flex-col items-center text-muted-foreground">
            <MicOff className="w-12 h-12 mb-2 opacity-20" />
            <p>Microphone is off</p>
          </div>
        )}
      </div>

      <div className="flex flex-wrap gap-2 justify-center">
        {!isRecording ? (
          <Button onClick={startRecording}>Start Microphone</Button>
        ) : (
          <>
            <Button variant="outline" onClick={stopRecording}>
              Stop Microphone
            </Button>
            {!isCapturing ? (
              <Button
                onClick={startCapturing}
                className="bg-red-500 hover:bg-red-600 border-none"
              >
                <Mic className="w-4 h-4 mr-2" />
                Record
              </Button>
            ) : (
              <Button variant="destructive" onClick={stopCapturing}>
                <Square className="w-4 h-4 mr-2" />
                Stop
              </Button>
            )}
          </>
        )}

        {lastRecording && (
          <Button variant="secondary" onClick={saveAudio} disabled={isSaving}>
            <Save
              className={`w - 4 h - 4 mr - 2 ${isSaving ? "animate-pulse" : ""} `}
            />
            Save Recording
          </Button>
        )}
      </div>

      {lastRecording && !isCapturing && (
        <div className="flex items-center gap-2 text-sm text-success p-2 bg-success/10 rounded-md">
          <Play className="w-4 h-4" />
          <span>Recording ready to save</span>
        </div>
      )}

      {isRecording && (
        <p className="text-sm text-muted-foreground animate-pulse flex items-center gap-2">
          {isCapturing ? (
            <span className="flex items-center gap-1 text-red-500">
              <span className="w-2 h-2 rounded-full bg-red-500 animate-ping" />
              Recording...
            </span>
          ) : (
            <>
              <Volume2 className="w-4 h-4" />
              Listening...
            </>
          )}
        </p>
      )}
    </div>
  );
}
