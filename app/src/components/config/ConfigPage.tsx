import { invoke } from "@tauri-apps/api/core";
import { RotateCcw, Save } from "lucide-react";
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
import type { FacepassConfig } from "@/types/config";
import { defaultConfig } from "@/types/config";
import { MethodsSection } from "./MethodsSection.tsx";
import { StrategySection } from "./StrategySection.tsx";
import { validateConfig } from "./validation";

export function ConfigPage() {
  const [config, setConfig] = useState<FacepassConfig>(defaultConfig);
  const [savedConfig, setSavedConfig] = useState<FacepassConfig>(defaultConfig);
  const [loading, setLoading] = useState(true);
  const [saving, setSaving] = useState(false);
  const [showPamConfirm, setShowPamConfirm] = useState(false);

  const initConfig = useCallback(async () => {
    try {
      setLoading(true);
      const loadedConfig = await invoke<FacepassConfig>("load_config");
      setConfig(loadedConfig);
      setSavedConfig(loadedConfig);
    } catch (err) {
      console.error("Failed to load config:", err);
      toast.error(`Failed to load config: ${err}`);
    } finally {
      setLoading(false);
    }
  }, []);

  useEffect(() => {
    initConfig();
  }, [initConfig]);

  async function saveConfig() {
    const isValid = await validateConfig(config);
    if (!isValid) return;

    // If PAM integration is enabled, show confirmation first
    if (config.strategy.pam_enabled && !showPamConfirm) {
      setShowPamConfirm(true);
      return;
    }

    await performSave();
  }

  async function performSave() {
    try {
      setSaving(true);
      setShowPamConfirm(false);
      await invoke("save_config", { config });
      setSavedConfig(config);

      // Apply PAM configuration
      try {
        await invoke("apply_pam_config");
        toast.success("Configuration and system settings saved successfully!");
      } catch (pamErr) {
        console.error("Failed to apply PAM config:", pamErr);
        // Special handling for user cancelation of pkexec
        if (pamErr?.toString().includes("cancelled")) {
          toast.warning("Config saved, but system integration was cancelled.");
        } else {
          toast.error(`Failed to apply system settings: ${pamErr}`);
        }
      }
    } catch (err) {
      console.error("Failed to save config:", err);
      toast.error(`Failed to save config: ${err}`);
    } finally {
      setSaving(false);
    }
  }

  function resetConfig() {
    setConfig(savedConfig);
    toast.info("Configuration reset to last saved state");
  }

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
            Facepass Configuration
          </h1>
          <p className="text-sm text-muted-foreground mt-1">
            Manage your authentication methods and execution strategies
          </p>
        </div>
        <div className="flex gap-2">
          <Button
            variant="outline"
            onClick={resetConfig}
            className="flex items-center gap-2 cursor-pointer"
          >
            <RotateCcw className="w-4 h-4" />
            Reset
          </Button>
          <Button
            onClick={saveConfig}
            disabled={saving}
            className="flex items-center gap-2 cursor-pointer"
          >
            <Save className="w-4 h-4" />
            {saving ? "Saving..." : "Save"}
          </Button>
        </div>
      </div>

      <div className="grid gap-6">
        <StrategySection
          strategy={config.strategy}
          onChange={(strategy: typeof config.strategy) =>
            setConfig({ ...config, strategy })
          }
        />
        <MethodsSection
          methods={config.methods}
          models={config.models}
          onChange={(methods: typeof config.methods) =>
            setConfig({ ...config, methods })
          }
        />
      </div>

      <Dialog open={showPamConfirm} onOpenChange={setShowPamConfirm}>
        <DialogContent>
          <DialogHeader>
            <DialogTitle>Elevated Permissions Required</DialogTitle>
            <DialogDescription>
              To enable System Sign-in Integration, FacePass needs to modify
              system files (PAM configuration). You will be asked for your
              password to authorize this change.
            </DialogDescription>
          </DialogHeader>
          <DialogFooter>
            <Button variant="outline" onClick={() => setShowPamConfirm(false)}>
              Cancel
            </Button>
            <Button onClick={performSave} disabled={saving}>
              {saving ? "Authorizing..." : "Continue"}
            </Button>
          </DialogFooter>
        </DialogContent>
      </Dialog>
    </div>
  );
}
