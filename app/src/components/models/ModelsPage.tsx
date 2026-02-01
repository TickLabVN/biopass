import { invoke } from "@tauri-apps/api/core";
import { open } from "@tauri-apps/plugin-dialog";
import {
  AlertCircle,
  CheckCircle2,
  Cpu,
  Download,
  Edit2,
  FileSearch,
  Loader2,
  MoreVertical,
  Plus,
  Trash2,
} from "lucide-react";
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
  DialogTrigger,
} from "@/components/ui/dialog";
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuItem,
  DropdownMenuTrigger,
} from "@/components/ui/dropdown-menu";
import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select";
import {
  Table,
  TableBody,
  TableCell,
  TableHead,
  TableHeader,
  TableRow,
} from "@/components/ui/table";
import { Tabs, TabsList, TabsTrigger } from "@/components/ui/tabs";
import type { FacepassConfig, ModelConfig } from "@/types/config";

export function ModelsPage() {
  const [models, setModels] = useState<ModelConfig[]>([]);
  const [loading, setLoading] = useState(true);
  const [isAddOpen, setIsAddOpen] = useState(false);
  const [addSource, setAddSource] = useState<"local" | "remote">("local");
  const [downloadUrl, setDownloadUrl] = useState("");
  const [isDownloading, setIsDownloading] = useState(false);
  const [newModel, setNewModel] = useState<ModelConfig>({
    path: "",
    name: "",
    type: "face",
  });
  const [editingIndex, setEditingIndex] = useState<number | null>(null);
  const [editingName, setEditingName] = useState("");
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

  const handleAddModel = async () => {
    if (addSource === "remote") {
      if (!downloadUrl) {
        toast.error("Please enter a download URL");
        return;
      }
      try {
        setIsDownloading(true);
        toast.info("Downloading model... this may take a moment.");
        const downloadedPath = await invoke<string>("download_model", {
          url: downloadUrl,
        });

        const updatedModels = [
          ...models,
          { ...newModel, path: downloadedPath },
        ];
        await saveModels(updatedModels);
        setIsAddOpen(false);
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
      const updatedModels = [...models, newModel];
      await saveModels(updatedModels);
      setIsAddOpen(false);
      setNewModel({ path: "", name: "", type: "face" });
    }
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
        <Dialog open={isAddOpen} onOpenChange={setIsAddOpen}>
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
                Register a new model by selecting a local file or downloading
                from a URL.
              </DialogDescription>
            </DialogHeader>

            <div className="grid gap-6 py-4">
              <Tabs
                value={addSource}
                onValueChange={(v) => setAddSource(v as "local" | "remote")}
                className="w-full"
              >
                <TabsList className="grid w-full grid-cols-2">
                  <TabsTrigger
                    value="local"
                    className="flex items-center gap-2"
                  >
                    <FileSearch className="w-3.5 h-3.5" />
                    Local File
                  </TabsTrigger>
                  <TabsTrigger
                    value="remote"
                    className="flex items-center gap-2"
                  >
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
                onClick={() => setIsAddOpen(false)}
                disabled={isDownloading}
              >
                Cancel
              </Button>
              <Button onClick={handleAddModel} disabled={isDownloading}>
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
      </div>

      <div className="rounded-xl border border-border/50 bg-card/50 backdrop-blur-sm overflow-hidden shadow-lg">
        <Table>
          <TableHeader>
            <TableRow className="bg-muted/30">
              <TableHead className="w-[80px]">Icon</TableHead>
              <TableHead>Name</TableHead>
              <TableHead>Type</TableHead>
              <TableHead>Status</TableHead>
              <TableHead className="max-w-[200px]">Path</TableHead>
              <TableHead className="text-right">Actions</TableHead>
            </TableRow>
          </TableHeader>
          <TableBody>
            {models.length === 0 ? (
              <TableRow>
                <TableCell
                  colSpan={6}
                  className="text-center py-12 text-muted-foreground"
                >
                  No models registered yet.
                </TableCell>
              </TableRow>
            ) : (
              models.map((model, index) => (
                <TableRow key={`${model.path}-${index}`}>
                  <TableCell>
                    <div className="w-10 h-10 rounded-lg bg-blue-500/10 flex items-center justify-center">
                      <Cpu className="w-5 h-5 text-blue-500" />
                    </div>
                  </TableCell>
                  <TableCell className="font-medium">
                    {model.name || (
                      <span className="text-muted-foreground italic">
                        Unnamed
                      </span>
                    )}
                  </TableCell>
                  <TableCell>
                    <span className="capitalize px-2 py-0.5 rounded-full bg-muted text-xs font-semibold">
                      {model.type}
                    </span>
                  </TableCell>
                  <TableCell>
                    {statusMap[model.path] === "checking" ||
                    !statusMap[model.path] ? (
                      <span className="text-xs text-muted-foreground animate-pulse">
                        Checking...
                      </span>
                    ) : statusMap[model.path] === "inuse" ? (
                      <span className="flex items-center gap-1.5 text-xs font-bold text-blue-500 bg-blue-500/10 px-2 py-1 rounded-full w-fit">
                        <Cpu className="w-3.5 h-3.5" />
                        In Use
                      </span>
                    ) : statusMap[model.path] === "available" ? (
                      <span className="flex items-center gap-1.5 text-xs font-bold text-emerald-500 bg-emerald-500/10 px-2 py-1 rounded-full w-fit">
                        <CheckCircle2 className="w-3.5 h-3.5" />
                        Available
                      </span>
                    ) : (
                      <span className="flex items-center gap-1.5 text-xs font-bold text-destructive bg-destructive/10 px-2 py-1 rounded-full w-fit">
                        <AlertCircle className="w-3.5 h-3.5" />
                        Missing
                      </span>
                    )}
                  </TableCell>
                  <TableCell
                    className="text-sm font-mono truncate max-w-[200px]"
                    title={model.path}
                  >
                    {model.path}
                  </TableCell>
                  <TableCell className="text-right">
                    <DropdownMenu>
                      <DropdownMenuTrigger asChild>
                        <Button
                          variant="ghost"
                          size="icon"
                          className="cursor-pointer"
                        >
                          <MoreVertical className="w-4 h-4" />
                        </Button>
                      </DropdownMenuTrigger>
                      <DropdownMenuContent align="end">
                        <DropdownMenuItem
                          className="cursor-pointer"
                          onClick={() => {
                            setEditingIndex(index);
                            setEditingName(model.name || "");
                          }}
                        >
                          <Edit2 className="w-4 h-4 mr-2" />
                          Rename
                        </DropdownMenuItem>
                        <DropdownMenuItem
                          className="text-destructive focus:bg-destructive/10 focus:text-destructive cursor-pointer"
                          onClick={() => handleDeleteModel(index)}
                        >
                          <Trash2 className="w-4 h-4 mr-2" />
                          Delete
                        </DropdownMenuItem>
                      </DropdownMenuContent>
                    </DropdownMenu>
                  </TableCell>
                </TableRow>
              ))
            )}
          </TableBody>
        </Table>
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
