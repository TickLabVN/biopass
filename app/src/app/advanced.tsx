import { createFileRoute } from "@tanstack/react-router";
import type { AdvancedConfig, FaceMethodConfig } from "@/types/config";
import { useConfigurationStore } from "../app/configuration/-stores/configuration-store";
import { AdvancedFaceSettings } from "../app/configuration/-components/face/AdvancedFaceSettings";

function AdvancedRouteComponent() {
  const config = useConfigurationStore((state) => state.config?.methods.face);
  const setFaceConfig = useConfigurationStore((state) => state.setFaceConfig);

  if (!config) return null;

  const faceConfig: FaceMethodConfig = config;

  function setAdvancedField<K extends keyof AdvancedConfig>(key: K, value: AdvancedConfig[K]) {
    setFaceConfig({ ...faceConfig, advanced: { ...faceConfig.advanced, [key]: value } } as FaceMethodConfig);
  }

  return (
    <div className="flex flex-col gap-6 w-full max-w-4xl mx-auto p-6">
      <div>
        <h1 className="text-3xl font-bold bg-linear-to-r from-primary to-purple-500 bg-clip-text text-transparent">
          Advanced Settings
        </h1>
        <p className="text-sm text-muted-foreground mt-1">
          Tuning parameters for face detection, IR capture, anti-spoofing, and more.
        </p>
      </div>

      <AdvancedFaceSettings
        advanced={config.advanced}
        onChange={setAdvancedField}
        alwaysOpen
      />
    </div>
  );
}

export const Route = createFileRoute("/advanced")({
  component: AdvancedRouteComponent,
});
