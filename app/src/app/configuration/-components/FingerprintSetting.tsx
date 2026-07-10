import { listen } from "@tauri-apps/api/event";
import { Trash2 } from "lucide-react";
import { useEffect, useState } from "react";
import { useFormContext, useWatch } from "react-hook-form";
import { toast } from "sonner";
import { cmd } from "@/commands";
import { Button } from "@/components/ui/button";
import { Label } from "@/components/ui/label";
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select";
import type { BiopassConfig } from "@/types/config";

export function FingerprintSetting() {
  const { getValues, setValue } = useFormContext<BiopassConfig>();
  const config = useWatch<BiopassConfig, "methods.fingerprint">({
    name: "methods.fingerprint",
  });
  const [selectedFinger, setSelectedFinger] = useState<string>("");
  const [isAdding, setIsAdding] = useState(false);
  const [username, setUsername] = useState<string>("");

  useEffect(() => {
    let canceled = false;

    const fetchUsername = async () => {
      try {
        const user = await cmd.system.getCurrentUsername();
        if (canceled) return;

        setUsername(user);

        // Sync enrolled fingers from backend
        const enrolledFingers = await cmd.fingerprint.listEnrolled(user);
        if (canceled) return;

        const currentConfig = getValues("methods.fingerprint");

        // Update local config if there's a mismatch (best effort)
        const currentFingerNames = currentConfig.fingers.map((f) => f.name);
        const needsSync =
          enrolledFingers.some((f) => !currentFingerNames.includes(f)) ||
          currentFingerNames.some((f) => !enrolledFingers.includes(f));

        if (needsSync) {
          const syncedFingers = enrolledFingers.map((name) => {
            const existing = currentConfig.fingers.find((f) => f.name === name);
            return (
              existing || { name, created_at: Math.floor(Date.now() / 1000) }
            );
          });
          setValue("methods.fingerprint.fingers", syncedFingers, {
            shouldDirty: false,
            shouldValidate: true,
          });
        }
      } catch (err) {
        console.error("Failed to sync fingerprints:", err);
      }
    };

    fetchUsername();

    return () => {
      canceled = true;
    };
  }, [getValues, setValue]);

  const fingerOptions = [
    "left-thumb",
    "left-index-finger",
    "left-middle-finger",
    "left-ring-finger",
    "left-little-finger",
    "right-thumb",
    "right-index-finger",
    "right-middle-finger",
    "right-ring-finger",
    "right-little-finger",
  ];

  const handleAdd = async () => {
    const currentConfig = getValues("methods.fingerprint");

    setIsAdding(true);
    const toastId = toast.loading(
      `Enrolling ${selectedFinger.replace(/-/g, " ")}... Please touch the sensor.`,
    );

    let scanCount = 0;
    const unlisten = await listen<{ done: boolean; status: string }>(
      "fingerprint-enroll-status",
      (event) => {
        if (event.payload.status === "enroll-stage-passed") {
          scanCount++;
          toast.loading(
            `Enrolling ${selectedFinger.replace(/-/g, " ")}... Scan ${scanCount} complete.`,
            { id: toastId },
          );
        }
      },
    );

    try {
      await cmd.fingerprint.enroll(username, selectedFinger);

      const formattedName = selectedFinger.replace(/-/g, " ");
      const capitalizedName =
        formattedName.charAt(0).toUpperCase() + formattedName.slice(1);
      toast.success(`${capitalizedName} enrolled!`, {
        id: toastId,
      });

      // The backend already saved this finger to config; just update the UI.
      setValue(
        "methods.fingerprint.fingers",
        [
          ...currentConfig.fingers,
          { name: selectedFinger, created_at: Math.floor(Date.now() / 1000) },
        ],
        {
          shouldDirty: false,
          shouldValidate: true,
        },
      );
      setSelectedFinger("");
    } catch (err) {
      toast.error(`Enrollment failed: ${err}`, { id: toastId });
    } finally {
      unlisten();
      setIsAdding(false);
    }
  };

  const handleDelete = async (fingerName: string) => {
    const currentConfig = getValues("methods.fingerprint");

    try {
      await cmd.fingerprint.remove(username, fingerName);
      const formattedName = fingerName.replace(/-/g, " ");
      const capitalizedName =
        formattedName.charAt(0).toUpperCase() + formattedName.slice(1);
      toast.success(`${capitalizedName} deleted`);

      // The backend already removed this finger from config; just update the UI.
      setValue(
        "methods.fingerprint.fingers",
        currentConfig.fingers.filter((f) => f.name !== fingerName),
        {
          shouldDirty: false,
          shouldValidate: true,
        },
      );
    } catch (err) {
      toast.error(`Delete failed: ${err}`);
    }
  };

  return (
    <div className="grid gap-4">
      <div className="p-4 rounded-lg bg-muted/50 border border-border/50">
        <h4 className="font-medium mb-3 text-sm">Registered Fingers</h4>
        {config.fingers.length > 0 ? (
          <div className="grid gap-2">
            {config.fingers.map((f) => (
              <div
                key={f.name}
                className="flex items-center justify-between p-2 bg-background rounded-lg border"
              >
                <div className="flex flex-col">
                  <span className="text-sm font-medium capitalize">
                    {f.name.replace(/-/g, " ")}
                  </span>
                </div>
                <button
                  type="button"
                  onClick={() => handleDelete(f.name)}
                  className="p-1 rounded hover:bg-destructive/20 text-destructive cursor-pointer transition-colors"
                >
                  <Trash2 className="w-4 h-4" />
                </button>
              </div>
            ))}
          </div>
        ) : (
          <p className="text-xs text-muted-foreground italic">
            No fingers registered yet.
          </p>
        )}
      </div>

      <div className="p-4 rounded-lg bg-muted/50 border border-border/50">
        <h4 className="font-medium mb-3 text-sm">Enroll New Finger</h4>
        <div className="grid gap-3">
          <div className="grid gap-2">
            <Label className="text-xs text-muted-foreground">
              Select Finger
            </Label>
            <Select value={selectedFinger} onValueChange={setSelectedFinger}>
              <SelectTrigger className="h-9">
                <SelectValue placeholder="Select Finger">
                  {selectedFinger && (
                    <span className="capitalize">
                      {selectedFinger.replace(/-/g, " ")}
                    </span>
                  )}
                </SelectValue>
              </SelectTrigger>
              <SelectContent>
                {fingerOptions.map((f) => (
                  <SelectItem
                    key={f}
                    value={f}
                    className="capitalize"
                    disabled={config.fingers.some((cf) => cf.name === f)}
                  >
                    {f.replace(/-/g, " ")}
                  </SelectItem>
                ))}
              </SelectContent>
            </Select>
          </div>

          <Button
            type="button"
            onClick={handleAdd}
            disabled={isAdding || !selectedFinger}
            className="w-full h-9 mt-1"
          >
            {isAdding ? "Enrolling..." : "Enroll Finger"}
          </Button>
        </div>
      </div>
    </div>
  );
}
