import { invoke } from "@tauri-apps/api/core";
import { useEffect, useState } from "react";
import { ModelStatus } from "@/components/models/ModelStatus";
import { Label } from "@/components/ui/label";
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select";
import { cn } from "@/lib/utils";
import type { ModelConfig } from "@/types/config";

interface Props {
  label: string;
  value: string;
  models: ModelConfig[];
  error: boolean;
  onChange: (value: string) => void;
}

export function ModelSelectField({
  label,
  value,
  models,
  error,
  onChange,
}: Props) {
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
                <div className="flex items-center gap-3 pr-6">
                  <span className="truncate">
                    {model.name || model.path.split("/").pop()}
                  </span>
                  <ModelStatus
                    status={statusMap[model.path]}
                    size="sm"
                    className="h-4"
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
