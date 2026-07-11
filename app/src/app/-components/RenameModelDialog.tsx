import { Loader2, Pencil } from "lucide-react";
import { useState } from "react";
import { toast } from "sonner";
import { cmd } from "@/commands";
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
import type { Model } from "@/types/config";

interface RenameModelDialogProps {
  model: Model;
  onRenamed: (model: Model) => void;
}

export function RenameModelDialog({
  model,
  onRenamed,
}: RenameModelDialogProps) {
  const [open, setOpen] = useState(false);
  const [name, setName] = useState(model.name);
  const [submitting, setSubmitting] = useState(false);

  async function handleSubmit() {
    if (!name.trim()) {
      toast.error("Please enter a model name");
      return;
    }
    setSubmitting(true);
    try {
      const updated = await cmd.models.rename(model.id, name.trim());
      toast.success("Model renamed");
      onRenamed(updated);
      setOpen(false);
    } catch (err) {
      toast.error(`Failed to rename model: ${err}`);
    } finally {
      setSubmitting(false);
    }
  }

  return (
    <Dialog
      open={open}
      onOpenChange={(next) => {
        if (!submitting) {
          setOpen(next);
          if (next) setName(model.name);
        }
      }}
    >
      <DialogTrigger asChild>
        <Button type="button" variant="ghost" size="icon" title="Rename model">
          <Pencil className="w-4 h-4" />
        </Button>
      </DialogTrigger>
      <DialogContent>
        <DialogHeader>
          <DialogTitle>Rename model</DialogTitle>
          <DialogDescription>
            Update the display name for this model. This does not affect any
            configuration referencing it.
          </DialogDescription>
        </DialogHeader>

        <div className="flex flex-col gap-2">
          <Label htmlFor="rename-model-name">Name</Label>
          <Input
            id="rename-model-name"
            value={name}
            onChange={(e) => setName(e.target.value)}
            disabled={submitting}
            onKeyDown={(e) => {
              if (e.key === "Enter") handleSubmit();
            }}
          />
        </div>

        <DialogFooter>
          <Button type="button" onClick={handleSubmit} disabled={submitting}>
            {submitting && <Loader2 className="w-4 h-4 mr-2 animate-spin" />}
            Save
          </Button>
        </DialogFooter>
      </DialogContent>
    </Dialog>
  );
}
