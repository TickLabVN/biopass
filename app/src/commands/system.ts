import { invokeCommand } from "./core";

function getCurrentUsername() {
  return invokeCommand<string>("get_current_username");
}

function hasConfigurationLock() {
  return invokeCommand<boolean>("has_configuration_lock");
}

function validateConfigurationLockKey(key: string) {
  return invokeCommand<boolean>("validate_configuration_lock_key", { key });
}

export const system = {
  getCurrentUsername,
  hasConfigurationLock,
  validateConfigurationLockKey,
};
