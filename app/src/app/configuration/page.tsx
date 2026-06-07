import { zodResolver } from "@hookform/resolvers/zod";
import { createFileRoute } from "@tanstack/react-router";
import { RotateCcw, Save } from "lucide-react";
import { useCallback, useEffect, useState } from "react";
import { FormProvider, useForm } from "react-hook-form";
import { toast } from "sonner";
import { cmd } from "@/commands";
import { Button } from "@/components/ui/button";
import type { BiopassConfig } from "@/types/config";
import { MethodConfig } from "./-components/MethodConfig";
import { StrategyConfig } from "./-components/StrategyConfig";
import { biopassConfigSchema, validateConfig } from "./-components/validation";

function cloneConfig(config: BiopassConfig): BiopassConfig {
  return {
    ...config,
    models: config.models.map((model) => ({ ...model })),
    strategy: {
      ...config.strategy,
      order: [...config.strategy.order],
      ignore_services: [...config.strategy.ignore_services],
    },
    methods: {
      face: {
        ...config.methods.face,
        detection: { ...config.methods.face.detection },
        recognition: { ...config.methods.face.recognition },
        anti_spoofing: {
          ...config.methods.face.anti_spoofing,
          model: { ...config.methods.face.anti_spoofing.model },
        },
      },
      fingerprint: {
        ...config.methods.fingerprint,
        fingers: config.methods.fingerprint.fingers.map((finger) => ({
          ...finger,
        })),
      },
    },
  };
}

function ConfigurationRouteComponent() {
  const [config, setConfig] = useState<BiopassConfig | null>(null);
  const [savedConfig, setSavedConfig] = useState<BiopassConfig | null>(null);
  const [loading, setLoading] = useState(true);
  const [saving, setSaving] = useState(false);

  useEffect(() => {
    let canceled = false;

    async function initializeConfig() {
      try {
        setLoading(true);
        const loadedConfig = await cmd.config.load();
        if (canceled) return;

        const nextConfig = cloneConfig(loadedConfig);
        setConfig(nextConfig);
        setSavedConfig(cloneConfig(nextConfig));
      } catch (err) {
        if (!canceled) {
          toast.error(`Failed to load config: ${err}`);
        }
      } finally {
        if (!canceled) {
          setLoading(false);
        }
      }
    }

    initializeConfig();

    return () => {
      canceled = true;
    };
  }, []);

  if (loading || !config) {
    return (
      <div className="flex items-center justify-center p-8">
        <div className="animate-spin rounded-full h-8 w-8 border-b-2 border-primary" />
      </div>
    );
  }

  return (
    <ConfigurationForm
      config={config}
      savedConfig={savedConfig}
      saving={saving}
      onConfigSaved={(nextConfig) => {
        const saved = cloneConfig(nextConfig);
        setConfig(saved);
        setSavedConfig(cloneConfig(saved));
      }}
      onReset={(nextConfig) => setConfig(cloneConfig(nextConfig))}
      setSaving={setSaving}
    />
  );
}

interface ConfigurationFormProps {
  config: BiopassConfig;
  savedConfig: BiopassConfig | null;
  saving: boolean;
  onConfigSaved: (config: BiopassConfig) => void;
  onReset: (config: BiopassConfig) => void;
  setSaving: (saving: boolean) => void;
}

function ConfigurationForm({
  config,
  savedConfig,
  saving,
  onConfigSaved,
  onReset,
  setSaving,
}: ConfigurationFormProps) {
  const form = useForm<BiopassConfig>({
    defaultValues: cloneConfig(config),
    resolver: zodResolver(biopassConfigSchema),
  });

  useEffect(() => {
    form.reset(cloneConfig(config));
  }, [config, form]);

  const handleSave = useCallback(
    async (values: BiopassConfig) => {
      const configToSave = cloneConfig(values);

      const isValid = await validateConfig(configToSave);
      if (!isValid) return;

      try {
        setSaving(true);
        await cmd.config.save(configToSave);
        const nextSavedConfig = cloneConfig(configToSave);
        onConfigSaved(nextSavedConfig);
        form.reset(nextSavedConfig);
        toast.success("Settings saved successfully!");
      } catch (err) {
        console.error("Failed to save config:", err);
        toast.error(`Failed to save config: ${err}`);
      } finally {
        setSaving(false);
      }
    },
    [form, onConfigSaved, setSaving],
  );

  useEffect(() => {
    function handleSaveShortcut(event: KeyboardEvent) {
      if (
        !(event.ctrlKey || event.metaKey) ||
        event.key.toLowerCase() !== "s"
      ) {
        return;
      }

      event.preventDefault();
      if (saving) return;

      form.handleSubmit(handleSave, () => {
        toast.error("Please fix validation errors before saving");
      })();
    }

    window.addEventListener("keydown", handleSaveShortcut);

    return () => {
      window.removeEventListener("keydown", handleSaveShortcut);
    };
  }, [form, saving, handleSave]);

  function handleReset() {
    if (!savedConfig) return;

    const resetValue = cloneConfig(savedConfig);
    onReset(resetValue);
    form.reset(resetValue);
    toast.info("Configuration reset to last saved state");
  }

  return (
    <FormProvider {...form}>
      <form
        onSubmit={form.handleSubmit(handleSave, () => {
          toast.error("Please fix validation errors before saving");
        })}
        className="flex flex-col gap-6 w-full max-w-4xl mx-auto p-6"
      >
        <div className="flex justify-between items-center">
          <div>
            <h1 className="text-3xl font-bold bg-linear-to-r from-primary to-purple-500 bg-clip-text text-transparent">
              Biopass Configuration
            </h1>
            <p className="text-sm text-muted-foreground mt-1">
              Manage your authentication methods and execution strategies
            </p>
          </div>
          <div className="flex gap-2">
            <Button
              type="button"
              variant="outline"
              onClick={handleReset}
              className="flex items-center gap-2 cursor-pointer"
            >
              <RotateCcw className="w-4 h-4" />
              Reset
            </Button>
            <Button
              type="submit"
              disabled={saving}
              className="flex items-center gap-2 cursor-pointer"
            >
              <Save className="w-4 h-4" />
              {saving ? "Saving..." : "Save"}
            </Button>
          </div>
        </div>

        <div className="grid gap-6">
          <StrategyConfig />
          <MethodConfig />
        </div>
      </form>
    </FormProvider>
  );
}

export const Route = createFileRoute("/configuration/")({
  component: ConfigurationRouteComponent,
});
