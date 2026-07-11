import type { Model, ModelType } from "@/types/config";
import { invokeCommand } from "./core";

function list(modelType?: ModelType) {
  return invokeCommand<Model[]>("list_models", {
    modelType: modelType ?? null,
  });
}

export const models = {
  list,
};
