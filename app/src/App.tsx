import { invoke } from "@tauri-apps/api/core";
import { getCurrentWindow } from "@tauri-apps/api/window";
import { Cpu, Laptop, Moon, Settings, Sun, User } from "lucide-react";
import { useTheme } from "next-themes";
import { useEffect, useState } from "react";
import { ConfigPage } from "@/components/config";
import { ModelsPage } from "@/components/models";
import { Button } from "@/components/ui/button";
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuItem,
  DropdownMenuTrigger,
} from "@/components/ui/dropdown-menu";
import type { FacepassConfig } from "@/types/config";
import logo from "./assets/logo.png";

type Tab = "config" | "models";

function App() {
  const [username, setUsername] = useState("");
  const [activeTab, setActiveTab] = useState<Tab>("config");
  const { setTheme, theme } = useTheme();

  useEffect(() => {
    // Listen for system theme changes from Tauri (more reliable on Linux)
    const window = getCurrentWindow();

    const sync = async () => {
      if (theme === "system") {
        const sysTheme = await window.theme();
        if (sysTheme)
          document.documentElement.classList.toggle(
            "dark",
            sysTheme === "dark",
          );
      }
    };

    sync();

    const unlisten = window.onThemeChanged(({ payload: newTheme }) => {
      if (theme === "system") {
        document.documentElement.classList.toggle("dark", newTheme === "dark");
      }
    });

    return () => {
      unlisten.then((fn) => fn());
    };
  }, [theme]);

  useEffect(() => {
    const loadUsername = async () => {
      try {
        const u = await invoke<string>("get_current_username");
        setUsername(u);
      } catch (err) {
        console.error("Failed to get username:", err);
      }
    };

    const loadInitialTheme = async () => {
      try {
        const config = await invoke<FacepassConfig>("load_config");
        if (config.appearance) {
          setTheme(config.appearance);
        }
      } catch (err) {
        console.error("Failed to load initial theme from config:", err);
      }
    };

    loadUsername();
    loadInitialTheme();
  }, [setTheme]);

  // Update theme in config when it changes manually via toggle
  useEffect(() => {
    const updateConfigTheme = async () => {
      try {
        const config = await invoke<FacepassConfig>("load_config");
        if (config.appearance !== theme) {
          config.appearance = theme ?? "dark";
          await invoke("save_config", { config });
        }
      } catch (err) {
        console.debug("Theme sync to config skipped or failed:", err);
      }
    };
    if (theme) updateConfigTheme();
  }, [theme]);

  return (
    <div className="min-h-screen bg-background text-foreground">
      {/* Navigation */}
      <nav className="sticky top-0 z-50 backdrop-blur-lg bg-background/80 border-b border-border">
        <div className="max-w-6xl mx-auto px-4">
          <div className="flex items-center justify-between h-16">
            <div className="flex items-center gap-8">
              <div className="flex items-center gap-3">
                <img src={logo} className="h-8" alt="Facepass logo" />
                <span className="font-bold text-lg hidden sm:inline-block">
                  Facepass
                </span>
              </div>

              {/* Tab Navigation */}
              <div className="flex items-center gap-2">
                <button
                  type="button"
                  onClick={() => setActiveTab("config")}
                  className={`flex items-center gap-2 px-3 py-1.5 rounded-md transition-colors cursor-pointer ${
                    activeTab === "config"
                      ? "bg-primary/10 text-primary"
                      : "text-muted-foreground hover:bg-muted hover:text-foreground"
                  }`}
                >
                  <Settings className="w-4 h-4" />
                  <span className="text-sm font-medium">Configuration</span>
                </button>
                <button
                  type="button"
                  onClick={() => setActiveTab("models")}
                  className={`flex items-center gap-2 px-3 py-1.5 rounded-md transition-colors cursor-pointer ${
                    activeTab === "models"
                      ? "bg-primary/10 text-primary"
                      : "text-muted-foreground hover:bg-muted hover:text-foreground"
                  }`}
                >
                  <Cpu className="w-4 h-4" />
                  <span className="text-sm font-medium">AI Models</span>
                </button>
              </div>
            </div>

            {username && (
              <div className="flex items-center gap-4">
                <DropdownMenu>
                  <DropdownMenuTrigger asChild>
                    <Button
                      variant="ghost"
                      size="icon"
                      className="cursor-pointer"
                    >
                      <Sun className="h-[1.2rem] w-[1.2rem] rotate-0 scale-100 transition-all dark:-rotate-90 dark:scale-0" />
                      <Moon className="absolute h-[1.2rem] w-[1.2rem] rotate-90 scale-0 transition-all dark:rotate-0 dark:scale-100" />
                      <span className="sr-only">Toggle theme</span>
                    </Button>
                  </DropdownMenuTrigger>
                  <DropdownMenuContent align="end">
                    <DropdownMenuItem
                      onClick={() => setTheme("light")}
                      className="cursor-pointer"
                    >
                      <Sun className="mr-2 h-4 w-4" />
                      <span>Light</span>
                    </DropdownMenuItem>
                    <DropdownMenuItem
                      onClick={() => setTheme("dark")}
                      className="cursor-pointer"
                    >
                      <Moon className="mr-2 h-4 w-4" />
                      <span>Dark</span>
                    </DropdownMenuItem>
                    <DropdownMenuItem
                      onClick={() => setTheme("system")}
                      className="cursor-pointer"
                    >
                      <Laptop className="mr-2 h-4 w-4" />
                      <span>System</span>
                    </DropdownMenuItem>
                  </DropdownMenuContent>
                </DropdownMenu>

                <div className="flex items-center gap-2 px-3 py-1.5 rounded-full bg-muted/50 border border-border/50">
                  <User className="w-4 h-4 text-muted-foreground" />
                  <span className="text-sm font-medium">{username}</span>
                </div>
              </div>
            )}
          </div>
        </div>
      </nav>

      {/* Content */}
      <main className="max-w-6xl mx-auto">
        {activeTab === "config" ? <ConfigPage /> : <ModelsPage />}
      </main>
    </div>
  );
}

export default App;
