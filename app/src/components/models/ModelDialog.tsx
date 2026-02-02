import { open } from "@tauri-apps/plugin-dialog";
import { Download, FileSearch, Loader2 } from "lucide-react";
import { useEffect, useState } from "react";
import { toast } from "sonner";
import { Button } from "@/components/ui/button";
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogFooter,
  DialogHeader,
  DialogTitle,
  DialogTrigger,
} from "@/components/ui/dialog";
import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select";
import { Tabs, TabsList, TabsTrigger } from "@/components/ui/tabs";
import type { ModelConfig } from "@/types/config";

interface ModelDialogProps {
  mode: "add" | "edit";
  initialData?: ModelConfig;
  isOpen: boolean;
  onOpenChange: (open: boolean) => void;
  onSubmit: (
    model: ModelConfig,
    isRemote: boolean,
    downloadUrl?: string,
  ) => Promise<void>;
  downloadModel?: (url: string) => Promise<string>;
  trigger?: React.ReactNode;
}

export function ModelDialog({
  mode,
  initialData,
  isOpen,
  onOpenChange,
  onSubmit,
  downloadModel,
  trigger,
}: ModelDialogProps) {
  const [addSource, setAddSource] = useState<"local" | "remote">("local");
  const [downloadUrl, setDownloadUrl] = useState("");
  const [isProcessing, setIsProcessing] = useState(false);
  const [model, setModel] = useState<ModelConfig>({
    path: "",
    name: "",
    type: "face",
  });

  useEffect(() => {
    if (isOpen) {
      if (mode === "edit" && initialData) {
        setModel(initialData);
        // If it's an edit, we typically don't show the "download" tab if it's already a local path,
        // but we can allow switching if desired. For simplicity, default to local.
        setAddSource("local");
      } else {
        setModel({ path: "", name: "", type: "face" });
        setDownloadUrl("");
        setAddSource("local");
      }
    }
  }, [isOpen, mode, initialData]);

  const handleSelectFile = async () => {
    try {
      const selected = await open({
        multiple: false,
      });
      if (selected && typeof selected === "string") {
        setModel({ ...model, path: selected });
      }
    } catch (err) {
      console.error("Failed to open file dialog:", err);
      toast.error("Failed to open file dialog");
    }
  };

  const handleAction = async () => {
    if (addSource === "remote") {
      if (!downloadUrl) {
        toast.error("Please enter a download URL");
        return;
      }
      try {
        setIsProcessing(true);
        if (mode === "add") {
          toast.info("Starting model download in background...");
          if (downloadModel) {
            const downloadedPath = await downloadModel(downloadUrl);
            await onSubmit(
              { ...model, path: downloadedPath },
              true,
              downloadUrl,
            );
          }
        } else {
          // Editing existing to a new download URL
          toast.info("Updating model with new download...");
          if (downloadModel) {
            const downloadedPath = await downloadModel(downloadUrl);
            await onSubmit(
              { ...model, path: downloadedPath },
              true,
              downloadUrl,
            );
          }
        }
        onOpenChange(false);
      } catch (err) {
        console.error("Failed to process model:", err);
        toast.error(`Failed to process model: ${err}`);
      } finally {
        setIsProcessing(false);
      }
    } else {
      if (!model.path) {
        toast.error("Please select a model file");
        return;
      }
      try {
        setIsProcessing(true);
        await onSubmit(model, false);
        onOpenChange(false);
      } catch (_err) {
        // Error toast is usually handled by parent
      } finally {
        setIsProcessing(false);
      }
    }
  };

  return (
    <Dialog open={isOpen} onOpenChange={onOpenChange}>
      {trigger && <DialogTrigger asChild>{trigger}</DialogTrigger>}
      <DialogContent className="sm:max-w-[500px]">
        <DialogHeader>
          <DialogTitle>
            {mode === "add" ? "Add New AI Model" : "Edit AI Model"}
          </DialogTitle>
          <DialogDescription>
            {mode === "add"
              ? "Register a new model by selecting a local file or downloading from a URL."
              : "Update your model configuration or replace the model file."}
          </DialogDescription>
        </DialogHeader>

        <div className="grid gap-6 py-4">
          <Tabs
            value={addSource}
            onValueChange={(v) => setAddSource(v as "local" | "remote")}
            className="w-full"
          >
            <TabsList className="grid w-full grid-cols-2">
              <TabsTrigger value="local" className="flex items-center gap-2">
                <FileSearch className="w-3.5 h-3.5" />
                Local File
              </TabsTrigger>
              <TabsTrigger value="remote" className="flex items-center gap-2">
                <Download className="w-3.5 h-3.5" />
                Download URL
              </TabsTrigger>
            </TabsList>
          </Tabs>

          <div className="grid gap-4">
            <div className="grid gap-2">
              <Label htmlFor="name">Friendly Name (Optional)</Label>
              <Input
                id="name"
                placeholder="e.g. My Custom Face Model"
                value={model.name || ""}
                onChange={(e) => setModel({ ...model, name: e.target.value })}
                disabled={isProcessing}
              />
            </div>

            <div className="grid gap-2">
              <Label htmlFor="type">Model Type</Label>
              <Select
                value={model.type}
                onValueChange={(value) =>
                  setModel({
                    ...model,
                    type: value as "face" | "voice",
                  })
                }
                disabled={isProcessing}
              >
                <SelectTrigger>
                  <SelectValue placeholder="Select type" />
                </SelectTrigger>
                <SelectContent>
                  <SelectItem value="face">Face Recognition</SelectItem>
                  <SelectItem value="voice">Voice Recognition</SelectItem>
                </SelectContent>
              </Select>
            </div>

            {addSource === "local" ? (
              <div className="grid gap-2">
                <Label>Model File</Label>
                <div className="flex items-center gap-3 p-2 rounded-md border border-input bg-background/50">
                  <Button
                    type="button"
                    variant="secondary"
                    size="sm"
                    onClick={handleSelectFile}
                    className="cursor-pointer shrink-0"
                    disabled={isProcessing}
                  >
                    Choose File
                  </Button>
                  <span
                    className="text-sm text-muted-foreground truncate flex-1"
                    title={model.path}
                  >
                    {model.path
                      ? model.path.split(/[\\/]/).pop()
                      : "No file chosen"}
                  </span>
                </div>
                {model.path && (
                  <p className="text-[10px] text-muted-foreground truncate px-1">
                    Full path: {model.path}
                  </p>
                )}
              </div>
            ) : (
              <div className="grid gap-2">
                <Label htmlFor="url">Download URL</Label>
                <Input
                  id="url"
                  placeholder="https://example.com/models/face.onnx"
                  value={downloadUrl}
                  onChange={(e) => setDownloadUrl(e.target.value)}
                  disabled={isProcessing}
                />
                <p className="text-[10px] text-muted-foreground">
                  Model will be saved to your application data directory.
                </p>
              </div>
            )}
          </div>
        </div>

        <DialogFooter>
          <Button
            variant="outline"
            onClick={() => onOpenChange(false)}
            disabled={isProcessing}
          >
            Cancel
          </Button>
          <Button onClick={handleAction} disabled={isProcessing}>
            {isProcessing ? (
              <>
                <Loader2 className="w-4 h-4 mr-2 animate-spin" />
                {addSource === "remote" ? "Starting..." : "Processing..."}
              </>
            ) : mode === "edit" ? (
              "Save Changes"
            ) : addSource === "remote" ? (
              "Download & Add"
            ) : (
              "Add Model"
            )}
          </Button>
        </DialogFooter>
      </DialogContent>
    </Dialog>
  );
}
