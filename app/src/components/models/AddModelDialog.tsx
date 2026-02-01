import { open } from "@tauri-apps/plugin-dialog";
import { Download, FileSearch, Loader2, Plus } from "lucide-react";
import { useState } from "react";
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

interface AddModelDialogProps {
  onAdd: (model: ModelConfig) => Promise<void>;
  downloadModel: (url: string) => Promise<string>;
}

export function AddModelDialog({ onAdd, downloadModel }: AddModelDialogProps) {
  const [isOpen, setIsOpen] = useState(false);
  const [addSource, setAddSource] = useState<"local" | "remote">("local");
  const [downloadUrl, setDownloadUrl] = useState("");
  const [isDownloading, setIsDownloading] = useState(false);
  const [newModel, setNewModel] = useState<ModelConfig>({
    path: "",
    name: "",
    type: "face",
  });

  const handleSelectFile = async () => {
    try {
      const selected = await open({
        multiple: false,
      });
      if (selected && typeof selected === "string") {
        setNewModel({ ...newModel, path: selected });
      }
    } catch (err) {
      console.error("Failed to open file dialog:", err);
      toast.error("Failed to open file dialog");
    }
  };

  const handleAddStart = async () => {
    if (addSource === "remote") {
      if (!downloadUrl) {
        toast.error("Please enter a download URL");
        return;
      }
      try {
        setIsDownloading(true);
        toast.info("Downloading model... this may take a moment.");

        const downloadedPath = await downloadModel(downloadUrl);

        await onAdd({ ...newModel, path: downloadedPath });

        setIsOpen(false);
        setNewModel({ path: "", name: "", type: "face" });
        setDownloadUrl("");
      } catch (err) {
        console.error("Failed to download model:", err);
        toast.error(`Download failed: ${err}`);
      } finally {
        setIsDownloading(false);
      }
    } else {
      if (!newModel.path) {
        toast.error("Please select a model file");
        return;
      }
      await onAdd(newModel);
      setIsOpen(false);
      setNewModel({ path: "", name: "", type: "face" });
    }
  };

  return (
    <Dialog open={isOpen} onOpenChange={setIsOpen}>
      <DialogTrigger asChild>
        <Button className="flex items-center gap-2">
          <Plus className="w-4 h-4" />
          Add Model
        </Button>
      </DialogTrigger>
      <DialogContent className="sm:max-w-[500px]">
        <DialogHeader>
          <DialogTitle>Add New AI Model</DialogTitle>
          <DialogDescription>
            Register a new model by selecting a local file or downloading from a
            URL.
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
                value={newModel.name || ""}
                onChange={(e) =>
                  setNewModel({ ...newModel, name: e.target.value })
                }
                disabled={isDownloading}
              />
            </div>

            <div className="grid gap-2">
              <Label htmlFor="type">Model Type</Label>
              <Select
                value={newModel.type}
                onValueChange={(value) =>
                  setNewModel({
                    ...newModel,
                    type: value as "face" | "voice",
                  })
                }
                disabled={isDownloading}
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
                  >
                    Choose File
                  </Button>
                  <span
                    className="text-sm text-muted-foreground truncate flex-1"
                    title={newModel.path}
                  >
                    {newModel.path
                      ? newModel.path.split(/[\\/]/).pop()
                      : "No file chosen"}
                  </span>
                </div>
                {newModel.path && (
                  <p className="text-[10px] text-muted-foreground truncate px-1">
                    Full path: {newModel.path}
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
                  disabled={isDownloading}
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
            onClick={() => setIsOpen(false)}
            disabled={isDownloading}
          >
            Cancel
          </Button>
          <Button onClick={handleAddStart} disabled={isDownloading}>
            {isDownloading ? (
              <>
                <Loader2 className="w-4 h-4 mr-2 animate-spin" />
                Downloading...
              </>
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
