import { invoke } from "@tauri-apps/api/core";
import { listen } from "@tauri-apps/api/event";
import { Cpu, Plus } from "lucide-react";
import { useCallback, useEffect, useState } from "react";
import { toast } from "sonner";
import { Button } from "@/components/ui/button";
import type { FacepassConfig, ModelConfig } from "@/types/config";
import { ModelCard } from "./ModelCard";
import { ModelDialog } from "./ModelDialog";

export function ModelsPage() {
  const [models, setModels] = useState<ModelConfig[]>([]);
  const [loading, setLoading] = useState(true);
  const [editingIndex, setEditingIndex] = useState<number | null>(null);
  const [isAddOpen, setIsAddOpen] = useState(false);
  const [statusMap, setStatusMap] = useState<
    Record<string, "checking" | "available" | "missing" | "inuse">
  >({});
  const [downloadProgress, setDownloadProgress] = useState<
    Record<string, number>
  >({});

  // Use a ref to open the dialog programmatically if needed, or manage state here
  // Since we extracted AddModelDialog, we can control it via a state if we want to trigger it from outside
  // But AddModelDialog manages its own open state with a trigger button.
  // We need to trigger it from the "empty state" button as well.
  // So we will lift the open state back up or expose it.
  // For simplicity, let's wrap AddModelDialog in a way we can control or reuse the trigger.
  // Actually, the previous design had the dialog wrapping the trigger.
  // Let's adjust AddModelDialog to accept `open` and `onOpenChange` props if we want external control,
  // OR just use a separate state for the empty view.
  // Let's refrain from overcomplicating and just use a state here and pass it down if needed,
  // but standard Dialog pattern uses Composition.

  // Redoing the strategy: We will keep the dialog logic inside AddModelDialog but maybe expose a trigger component?
  // Or simpler: Just render `AddModelDialog` where the button should be.
  // But for the "Empty State" button, it's separate.
  // Let's make `AddModelDialog` accept an `open` prop control or just render it alongside.

  // To support the "Add Model" button in the empty state, we need to share the toggle state.
  const [_unused, _setUnused] = useState(false);

  const checkModelsStatus = useCallback(async (modelList: ModelConfig[]) => {
    const newStatuses: Record<
      string,
      "checking" | "available" | "missing" | "inuse"
    > = {};

    // Set all to checking initially
    for (const m of modelList) newStatuses[m.path] = "checking";
    setStatusMap({ ...newStatuses });

    try {
      const config = await invoke<FacepassConfig>("load_config");
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
      const config = await invoke<FacepassConfig>("load_config");
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

    const unlistenProgress = listen<{
      url: string;
      path: string;
      progress: number;
    }>("download-progress", (event) => {
      setDownloadProgress((prev) => ({
        ...prev,
        [event.payload.path]: event.payload.progress,
      }));
    });

    const unlistenFinished = listen<{
      url: string;
      path?: string;
      error?: string;
    }>("download-finished", (event) => {
      if (event.payload.path) {
        const path = event.payload.path;
        setDownloadProgress((prev) => {
          const next = { ...prev };
          delete next[path];
          return next;
        });
      }

      if (event.payload.error) {
        toast.error(`Download failed: ${event.payload.error}`);
      } else {
        toast.success("Download complete!");
        loadConfig(); // Refresh to show the new model as available
      }
    });

    return () => {
      unlistenProgress.then((u) => u());
      unlistenFinished.then((u) => u());
    };
  }, [loadConfig]);

  const saveModels = async (updatedModels: ModelConfig[]) => {
    try {
      const config = await invoke<FacepassConfig>("load_config");
      const updatedConfig = { ...config, models: updatedModels };
      await invoke("save_config", { config: updatedConfig });
      setModels(updatedModels);
      checkModelsStatus(updatedModels);
      toast.success("Models updated successfully");
    } catch (err) {
      console.error("Failed to save models:", err);
      toast.error("Failed to save models");
    }
  };

  const handleAddModel = async (newModel: ModelConfig) => {
    // Validation: Check for duplicate local paths
    if (models.some((m) => m.path === newModel.path)) {
      toast.error("A model with this file path already exists.");
      throw new Error("Duplicate path");
    }

    const updatedModels = [...models, newModel];
    await saveModels(updatedModels);
  };

  const handleDownloadModel = async (url: string): Promise<string> => {
    return await invoke<string>("download_model", { url });
  };

  const handleUpdateModel = async (updatedModel: ModelConfig) => {
    if (editingIndex === null) return;

    // Validation: Check for duplicate local paths (excluding the one being edited)
    if (
      models.some((m, i) => i !== editingIndex && m.path === updatedModel.path)
    ) {
      toast.error("Another model with this file path already exists.");
      throw new Error("Duplicate path");
    }

    const updatedModels = [...models];
    updatedModels[editingIndex] = updatedModel;
    await saveModels(updatedModels);
    setEditingIndex(null);
  };

  const handleDeleteModel = async (index: number) => {
    const modelToDelete = models[index];

    try {
      const config = await invoke<FacepassConfig>("load_config");
      const usages: string[] = [];

      if (config.methods.face.detection.model === modelToDelete.path) {
        usages.push("Face Detection");
      }
      if (config.methods.face.recognition.model === modelToDelete.path) {
        usages.push("Face Recognition");
      }
      if (
        config.methods.face.anti_spoofing.enable &&
        config.methods.face.anti_spoofing.model === modelToDelete.path
      ) {
        usages.push("Face Anti-Spoofing");
      }
      if (config.methods.voice.model === modelToDelete.path) {
        usages.push("Voice Recognition");
      }

      if (usages.length > 0) {
        toast.error(
          `Cannot delete model "${modelToDelete.name || modelToDelete.path.split(/[\\/]/).pop()}". It is currently in use by: ${usages.join(", ")}.`,
        );
        return;
      }

      const updatedModels = models.filter((_, i) => i !== index);
      await saveModels(updatedModels);

      // Delete the actual file from disk
      try {
        await invoke("delete_file", { path: modelToDelete.path });
      } catch (err) {
        console.error("Failed to delete model file:", err);
        // We don't toast error here because the model is already gone from config
      }
    } catch (err) {
      console.error("Failed to check model usage:", err);
      toast.error("Failed to verify model usage before deletion");
    }
  };

  if (loading) {
    return (
      <div className="flex items-center justify-center p-8">
        <div className="animate-spin rounded-full h-8 w-8 border-b-2 border-primary" />
      </div>
    );
  }

  return (
    <div className="flex flex-col gap-6 w-full max-w-4xl mx-auto p-6">
      <div className="flex justify-between items-center">
        <div>
          <h1 className="text-3xl font-bold bg-linear-to-r from-primary to-purple-500 bg-clip-text text-transparent">
            AI Model Management
          </h1>
          <p className="text-sm text-muted-foreground mt-1">
            Configure and manage the AI models used for authentication
          </p>
        </div>

        <ModelDialog
          mode="add"
          isOpen={isAddOpen}
          onOpenChange={setIsAddOpen}
          onSubmit={handleAddModel}
          downloadModel={handleDownloadModel}
          trigger={
            <Button className="flex items-center gap-2">
              <Plus className="w-4 h-4" />
              Add Model
            </Button>
          }
        />
      </div>

      <div className="flex flex-col gap-4">
        {models.length === 0 ? (
          <div className="col-span-full flex flex-col items-center justify-center p-12 text-center rounded-xl border border-dashed border-border/50 bg-card/50">
            <div className="w-12 h-12 rounded-full bg-muted flex items-center justify-center mb-4">
              <Cpu className="w-6 h-6 text-muted-foreground" />
            </div>
            <h3 className="font-semibold text-lg">No models registered</h3>
            <p className="text-sm text-muted-foreground mt-1 max-w-sm">
              Add your first AI model to start using facial or voice recognition
              features.
            </p>
            <Button
              className="mt-4 flex items-center gap-2"
              onClick={() => setIsAddOpen(true)}
            >
              <Plus className="w-4 h-4" />
              Add Model
            </Button>
          </div>
        ) : (
          models.map((model, index) => (
            <ModelCard
              key={`${model.path}-${index}`}
              model={model}
              status={
                downloadProgress[model.path] !== undefined
                  ? "downloading"
                  : statusMap[model.path]
              }
              progress={downloadProgress[model.path] || 0}
              onEdit={() => setEditingIndex(index)}
              onDelete={() => handleDeleteModel(index)}
            />
          ))
        )}
      </div>

      <ModelDialog
        mode="edit"
        isOpen={editingIndex !== null}
        onOpenChange={(open) => !open && setEditingIndex(null)}
        initialData={editingIndex !== null ? models[editingIndex] : undefined}
        onSubmit={handleUpdateModel}
        downloadModel={handleDownloadModel}
      />
    </div>
  );
}
