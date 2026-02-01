import { invoke } from "@tauri-apps/api/core";
import { RotateCcw, Save, User } from "lucide-react";
import { useCallback, useEffect, useState } from "react";
import { toast } from "sonner";
import { Button } from "@/components/ui/button";
import type { FacepassConfig } from "@/types/config";
import { defaultConfig } from "@/types/config";
import { MethodsSection } from "./MethodsSection.tsx";
import { StrategySection } from "./StrategySection.tsx";

export function ConfigPage() {
  const [config, setConfig] = useState<FacepassConfig>(defaultConfig);
  const [loading, setLoading] = useState(true);
  const [saving, setSaving] = useState(false);
  const [username, setUsername] = useState("");

  const initUser = useCallback(async () => {
    try {
      const currentUser = await invoke<string>("get_current_username");
      setUsername(currentUser);

      // Load config
      try {
        setLoading(true);
        const loadedConfig = await invoke<FacepassConfig>("load_config");
        setConfig(loadedConfig);
      } catch (err) {
        console.error("Failed to load config:", err);
        toast.error(`Failed to load config: ${err}`);
      } finally {
        setLoading(false);
      }
    } catch (err) {
      console.error("Failed to get username:", err);
      toast.error(`Failed to get username: ${err}`);
      setLoading(false);
    }
  }, []);

  useEffect(() => {
    initUser();
  }, [initUser]);

  async function saveConfig() {
    try {
      setSaving(true);
      await invoke("save_config", { config });
      toast.success("Configuration saved successfully!");
    } catch (err) {
      console.error("Failed to save config:", err);
      toast.error(`Failed to save config: ${err}`);
    } finally {
      setSaving(false);
    }
  }

  function resetConfig() {
    setConfig(defaultConfig);
    toast.info("Configuration reset to defaults");
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
          <p className="text-sm text-muted-foreground flex items-center gap-1 mt-1">
            <User className="w-3 h-3" />
            <span className="font-medium">{username}</span>
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
          onChange={(methods: typeof config.methods) =>
            setConfig({ ...config, methods })
          }
        />
      </div>
    </div>
  );
}
