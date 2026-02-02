import { revealItemInDir } from "@tauri-apps/plugin-opener";
import { Cpu, Edit2, MoreVertical, Trash2 } from "lucide-react";
import { toast } from "sonner";
import { Button } from "@/components/ui/button";
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuItem,
  DropdownMenuTrigger,
} from "@/components/ui/dropdown-menu";
import {
  Tooltip,
  TooltipContent,
  TooltipProvider,
  TooltipTrigger,
} from "@/components/ui/tooltip";
import type { ModelConfig } from "@/types/config";
import { ModelStatus, type ModelStatusType } from "./ModelStatus";

interface ModelCardProps {
  model: ModelConfig;
  status: ModelStatusType;
  progress?: number;
  onEdit: () => void;
  onDelete: () => void;
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
            className="text-[10px] font-mono text-muted-foreground opacity-60 truncate max-w-[150px] bg-muted/50 px-1.5 py-0.5 rounded hover:opacity-100 hover:bg-primary/10 hover:text-primary transition-all cursor-pointer"
          >
            {path.split(/[/]/).pop()}
          </button>
        </TooltipTrigger>
        <TooltipContent
          side="bottom"
          align="end"
          className="max-w-[300px] break-all"
        >
          <p className="font-mono text-xs">{path}</p>
        </TooltipContent>
      </Tooltip>
    </TooltipProvider>
  );
}

export function ModelCard({
  model,
  status,
  progress = 0,
  onEdit,
  onDelete,
}: ModelCardProps) {
  return (
    <div className="group relative flex flex-col gap-4 p-5 rounded-xl border border-border bg-card/50 backdrop-blur-sm hover:border-primary/20 hover:shadow-lg transition-all duration-300">
      <div className="flex sm:flex-row sm:items-start justify-between gap-4">
        <div className="flex items-center gap-3">
          <div className="w-10 h-10 rounded-lg bg-linear-to-br from-blue-500/10 to-indigo-500/10 flex items-center justify-center border border-blue-500/10 group-hover:border-blue-500/30 transition-colors">
            <Cpu className="w-5 h-5 text-blue-600 dark:text-blue-400" />
          </div>
          <div>
            <div className="flex items-center gap-4">
              <h3
                className="font-semibold leading-none truncate max-w-[140px]"
                title={model.name}
              >
                {model.name || (
                  <span className="text-muted-foreground italic">Unnamed</span>
                )}
              </h3>
              <p className="text-xs text-muted-foreground capitalize mt-1 block">
                {model.type} Recognition
              </p>
            </div>
            <ModelFileFolderButton path={model.path} />
          </div>
        </div>

        <div className="flex items-center gap-1">
          <ModelStatus status={status} progress={progress} />

          <DropdownMenu>
            <DropdownMenuTrigger asChild>
              <Button
                variant="ghost"
                size="icon"
                className="h-8 w-8 text-muted-foreground hover:text-foreground -mr-2"
              >
                <MoreVertical className="w-4 h-4" />
              </Button>
            </DropdownMenuTrigger>
            <DropdownMenuContent align="end">
              <DropdownMenuItem className="cursor-pointer" onClick={onEdit}>
                <Edit2 className="w-4 h-4 mr-2" />
                Edit
              </DropdownMenuItem>
              <DropdownMenuItem
                className="text-destructive focus:bg-destructive/10 focus:text-destructive cursor-pointer"
                onClick={onDelete}
              >
                <Trash2 className="w-4 h-4 mr-2" />
                Delete
              </DropdownMenuItem>
            </DropdownMenuContent>
          </DropdownMenu>
        </div>
      </div>
    </div>
  );
}
