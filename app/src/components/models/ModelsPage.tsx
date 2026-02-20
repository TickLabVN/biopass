import { invoke } from "@tauri-apps/api/core";
import { Cpu } from "lucide-react";
import { useCallback, useEffect, useState } from "react";
import { toast } from "sonner";
import type { BiopassConfig, ModelConfig } from "@/types/config";
import { ModelCard } from "./ModelCard";

export function ModelsPage() {
  const [models, setModels] = useState<ModelConfig[]>([]);
  const [loading, setLoading] = useState(true);
  const [statusMap, setStatusMap] = useState<
    Record<string, "checking" | "available" | "missing" | "inuse">
  >({});

  const checkModelsStatus = useCallback(async (modelList: ModelConfig[]) => {
    const newStatuses: Record<
      string,
      "checking" | "available" | "missing" | "inuse"
    > = {};

    // Set all to checking initially
    for (const m of modelList) newStatuses[m.path] = "checking";
    setStatusMap({ ...newStatuses });

    try {
      const config = await invoke<BiopassConfig>("load_config");
      const inUsePaths = new Set<string>();

      // Collect all used paths
      if (config.methods.face.detection.model)
        inUsePaths.add(config.methods.face.detection.model);
      if (config.methods.face.recognition.model)
        inUsePaths.add(config.methods.face.recognition.model);
      if (
        config.methods.face.anti_spoofing.enable &&
        config.methods.face.anti_spoofing.model
      ) {
        inUsePaths.add(config.methods.face.anti_spoofing.model);
      }
      if (config.methods.voice.model)
        inUsePaths.add(config.methods.voice.model);

      const checks = modelList.map(async (model) => {
        try {
          const exists = await invoke<boolean>("check_file_exists", {
            path: model.path,
          });

          if (!exists) {
            newStatuses[model.path] = "missing";
          } else if (inUsePaths.has(model.path)) {
            newStatuses[model.path] = "inuse";
          } else {
            newStatuses[model.path] = "available";
          }
        } catch (err) {
          console.error(`Status check failed for ${model.path}:`, err);
          newStatuses[model.path] = "missing";
        }
      });

      await Promise.all(checks);
      setStatusMap({ ...newStatuses });
    } catch (err) {
      console.error("Failed to check model usage:", err);
    }
  }, []);

  const loadConfig = useCallback(async () => {
    try {
      setLoading(true);
      const config = await invoke<BiopassConfig>("load_config");
      setModels(config.models || []);
      checkModelsStatus(config.models || []);
    } catch (err) {
      console.error("Failed to load models:", err);
      toast.error("Failed to load models");
    } finally {
      setLoading(false);
    }
  }, [checkModelsStatus]);

  useEffect(() => {
    loadConfig();
  }, [loadConfig]);

  if (loading) {
    return (
      <div className="flex items-center justify-center p-8">
        <div className="animate-spin rounded-full h-8 w-8 border-b-2 border-primary" />
      </div>
    );
  }

  return (
    <div className="flex flex-col gap-6 w-full max-width-4xl mx-auto p-6">
      <div className="flex justify-between items-center">
        <div>
          <h1 className="text-3xl font-bold bg-linear-to-r from-primary to-purple-500 bg-clip-text text-transparent">
            AI Model Management
          </h1>
          <p className="text-sm text-muted-foreground mt-1">
            View AI models used for authentication.
          </p>
        </div>
      </div>

      <div className="flex flex-col gap-4">
        {models.length === 0 ? (
          <div className="col-span-full flex flex-col items-center justify-center p-12 text-center rounded-xl border border-dashed border-border/50 bg-card/50">
            <div className="w-12 h-12 rounded-full bg-muted flex items-center justify-center mb-4">
              <Cpu className="w-6 h-6 text-muted-foreground" />
            </div>
            <h3 className="font-semibold text-lg">No models registered</h3>
          </div>
        ) : (
          models.map((model, index) => (
            <ModelCard
              key={`${model.path}-${index}`}
              model={model}
              status={statusMap[model.path]}
            />
          ))
        )}
      </div>
    </div>
  );
}
