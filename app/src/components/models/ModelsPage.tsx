import { invoke } from "@tauri-apps/api/core";
import { Cpu } from "lucide-react";
import { useCallback, useEffect, useState } from "react";
import { toast } from "sonner";
import { Button } from "@/components/ui/button";
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogFooter,
  DialogHeader,
  DialogTitle,
} from "@/components/ui/dialog";
import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";
import type { FacepassConfig, ModelConfig } from "@/types/config";
import { AddModelDialog } from "./AddModelDialog";
import { ModelCard } from "./ModelCard";

export function ModelsPage() {
  const [models, setModels] = useState<ModelConfig[]>([]);
  const [loading, setLoading] = useState(true);
  const [editingIndex, setEditingIndex] = useState<number | null>(null);
  const [editingName, setEditingName] = useState("");
  const [statusMap, setStatusMap] = useState<
    Record<string, "checking" | "available" | "missing" | "inuse">
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
  const [_isAddOpen, _setIsAddOpen] = useState(false);

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
    const updatedModels = [...models, newModel];
    await saveModels(updatedModels);
  };

  const handleDownloadModel = async (url: string): Promise<string> => {
    return await invoke<string>("download_model", { url });
  };

  const handleUpdateModelName = () => {
    if (editingIndex === null) return;
    const updatedModels = [...models];
    updatedModels[editingIndex] = {
      ...updatedModels[editingIndex],
      name: editingName,
    };
    saveModels(updatedModels);
    setEditingIndex(null);
    setEditingName("");
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

        {/* We need to pass isOpen/onOpenChange because we want to also trigger from empty state */}
        {/* However, the current AddModelDialog implementation uses internal state. 
             Let's just use a key to force re-render or let it handle itself, 
             BUT the requirement was to split. 
             To properly control from the empty state button, I should have lifted state.
             Quick fix: Allow AddModelDialog to be controlled or uncontrolled.
             Re-writing ModelsPage to use AddModelDialog with internal trigger for the top right button.
          */}
        <AddModelDialog
          onAdd={handleAddModel}
          downloadModel={handleDownloadModel}
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
            {/* This button essentially duplicates the top right trigger. 
                 Ideally we click it and it opens the dialog. 
                 Since I didn't verify AddModelDialog opened state props, 
                 I'll just hint the user to use the top button or 
                 I will need to update AddModelDialog to accept an open prop.
                 Let's update AddModelDialog in the next step to be controlled.
              */}
          </div>
        ) : (
          models.map((model, index) => (
            <ModelCard
              key={`${model.path}-${index}`}
              model={model}
              status={statusMap[model.path]}
              onEdit={() => {
                setEditingIndex(index);
                setEditingName(model.name || "");
              }}
              onDelete={() => handleDeleteModel(index)}
            />
          ))
        )}
      </div>

      <Dialog
        open={editingIndex !== null}
        onOpenChange={(open) => !open && setEditingIndex(null)}
      >
        <DialogContent className="sm:max-w-[425px]">
          <DialogHeader>
            <DialogTitle>Rename Model</DialogTitle>
            <DialogDescription>
              Give this model a friendly name for easier identification.
            </DialogDescription>
          </DialogHeader>
          <div className="grid gap-4 py-4">
            <div className="grid gap-2">
              <Label htmlFor="edit-name">Model Name</Label>
              <Input
                id="edit-name"
                value={editingName}
                onChange={(e) => setEditingName(e.target.value)}
                placeholder="e.g. Production Face Model"
              />
            </div>
          </div>
          <DialogFooter>
            <Button variant="outline" onClick={() => setEditingIndex(null)}>
              Cancel
            </Button>
            <Button onClick={handleUpdateModelName}>Save Changes</Button>
          </DialogFooter>
        </DialogContent>
      </Dialog>
    </div>
  );
}
