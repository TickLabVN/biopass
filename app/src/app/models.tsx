import { createFileRoute } from "@tanstack/react-router";
import { revealItemInDir } from "@tauri-apps/plugin-opener";
import { Cpu } from "lucide-react";
import { useCallback, useEffect, useState } from "react";
import { toast } from "sonner";
import { cmd } from "@/commands";
import {
  Tooltip,
  TooltipContent,
  TooltipProvider,
  TooltipTrigger,
} from "@/components/ui/tooltip";
import type { ModelConfig } from "@/types/config";
import { ModelStatus, type ModelStatusType } from "./-components/ModelStatus";

interface ModelCardProps {
  model: ModelConfig;
  status: ModelStatusType;
}

function ModelFileFolderButton({ path }: { path: string }) {
  const handleOpenFileFolder = async (path: string) => {
    try {
      await revealItemInDir(path);
    } catch (err) {
      console.error("Failed to open file location:", err);
      toast.error(`Failed to open file location: ${err}`);
    }
  };
  return (
    <TooltipProvider>
      <Tooltip delayDuration={300}>
        <TooltipTrigger asChild>
          <button
            type="button"
            onClick={() => handleOpenFileFolder(path)}
            className="text-[10px] font-mono text-muted-foreground opacity-60 truncate max-w-37.5 bg-muted/50 px-1.5 py-0.5 rounded hover:opacity-100 hover:bg-primary/10 hover:text-primary transition-all cursor-pointer"
          >
            {path.split(/[/]/).pop()}
          </button>
        </TooltipTrigger>
        <TooltipContent
          side="bottom"
          align="end"
          className="max-w-75 break-all"
        >
          <p className="font-mono text-xs">{path}</p>
        </TooltipContent>
      </Tooltip>
    </TooltipProvider>
  );
}

function ModelCard({ model, status }: ModelCardProps) {
  const filename = model.path.split(/[/]/).pop() || model.path;

  return (
    <div className="group relative flex flex-col gap-4 p-5 rounded-xl border border-border bg-linear-to-b from-card to-muted/20 shadow-sm hover:border-primary/30 hover:shadow-md transition-all duration-300">
      <div className="flex sm:flex-row sm:items-start justify-between gap-4">
        <div className="flex items-center gap-3">
          <div className="w-10 h-10 rounded-lg bg-linear-to-br from-blue-500/10 to-indigo-500/10 flex items-center justify-center border border-blue-500/10 group-hover:border-blue-500/30 transition-colors">
            <Cpu className="w-5 h-5 text-blue-600 dark:text-blue-400" />
          </div>
          <div>
            <div className="flex items-center gap-4">
              <h3
                className="font-semibold leading-none truncate max-w-100"
                title={filename}
              >
                {filename}
              </h3>
              <p className="text-xs text-muted-foreground capitalize mt-1 block">
                {model.type}
              </p>
            </div>
            <ModelFileFolderButton path={model.path} />
          </div>
        </div>

        <div className="flex items-center gap-1">
          <ModelStatus status={status} />
        </div>
      </div>
    </div>
  );
}

function ModelsRouteComponent() {
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

    for (const model of modelList) newStatuses[model.path] = "checking";
    setStatusMap({ ...newStatuses });

    try {
      const config = await cmd.config.load();
      const inUsePaths = new Set<string>();

      if (config.methods.face.detection.model) {
        inUsePaths.add(config.methods.face.detection.model);
      }
      if (config.methods.face.recognition.model) {
        inUsePaths.add(config.methods.face.recognition.model);
      }
      if (
        config.methods.face.anti_spoofing.enable &&
        config.methods.face.anti_spoofing.model.path
      ) {
        inUsePaths.add(config.methods.face.anti_spoofing.model.path);
      }
      if (config.methods.voice.model) {
        inUsePaths.add(config.methods.voice.model);
      }

      const checks = modelList.map(async (model) => {
        try {
          const exists = await cmd.file.exists(model.path);
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

  const loadModels = useCallback(async () => {
    try {
      setLoading(true);
      const config = await cmd.config.load();
      const loadedModels = config.models || [];
      setModels(loadedModels);
      await checkModelsStatus(loadedModels);
    } catch (err) {
      console.error("Failed to load models:", err);
      toast.error("Failed to load models");
    } finally {
      setLoading(false);
    }
  }, [checkModelsStatus]);

  useEffect(() => {
    loadModels();
  }, [loadModels]);

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
          models.map((model) => (
            <ModelCard
              key={model.path}
              model={model}
              status={statusMap[model.path]}
            />
          ))
        )}
      </div>
    </div>
  );
}

export const Route = createFileRoute("/models")({
  component: ModelsRouteComponent,
});
