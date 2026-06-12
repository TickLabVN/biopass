import {
  createRootRoute,
  Link,
  Outlet,
  useRouterState,
} from "@tanstack/react-router";
import { TanStackRouterDevtools } from "@tanstack/react-router-devtools";
import { getCurrentWindow } from "@tauri-apps/api/window";
import { Cpu, Laptop, Moon, Settings, Sun, User } from "lucide-react";
import { useTheme } from "next-themes";
import { useEffect, useRef, useState } from "react";
import { toast } from "sonner";
import logo from "@/assets/logo.png";
import { cmd } from "@/commands";
import { Button } from "@/components/ui/button";
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogFooter,
  DialogHeader,
  DialogTitle,
} from "@/components/ui/dialog";
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuItem,
  DropdownMenuTrigger,
} from "@/components/ui/dropdown-menu";
import { Input } from "@/components/ui/input";

function App() {
  const [username, setUsername] = useState("");
  const [biopassKey, setBiopassKey] = useState("");
  const [isCheckingLock, setIsCheckingLock] = useState(true);
  const [isLockRequired, setIsLockRequired] = useState(false);
  const [isValidatingKey, setIsValidatingKey] = useState(false);
  const { setTheme, theme } = useTheme();
  const pathname = useRouterState({
    select: (state) => state.location.pathname,
  });

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

  const initialized = useRef(false);

  useEffect(() => {
    const checkConfigurationLock = async () => {
      try {
        const lockEnabled = await cmd.system.hasConfigurationLock();
        setIsLockRequired(lockEnabled);
      } catch (err) {
        console.error("Failed to check configuration lock:", err);
        setIsLockRequired(false);
      } finally {
        setIsCheckingLock(false);
      }
    };

    checkConfigurationLock();
  }, []);

  useEffect(() => {
    const loadUsername = async () => {
      if (isLockRequired) return;

      try {
        const u = await cmd.system.getCurrentUsername();
        setUsername(u);
      } catch (err) {
        console.error("Failed to get username:", err);
      }
    };

    const loadInitialTheme = async () => {
      if (isLockRequired) return;
      if (initialized.current) return;
      initialized.current = true;

      try {
        const config = await cmd.config.load();
        if (config.appearance) {
          setTheme(config.appearance);
        }
      } catch (err) {
        console.error("Failed to load initial theme from config:", err);
      }
    };

    loadUsername();
    loadInitialTheme();
  }, [setTheme, isLockRequired]);

  // Update theme in config when it changes manually via toggle
  useEffect(() => {
    if (isLockRequired) return;

    const updateConfigTheme = async () => {
      try {
        const config = await cmd.config.load();
        if (config.appearance !== theme) {
          config.appearance = theme ?? "dark";
          await cmd.config.save(config);
        }
      } catch (err) {
        console.debug("Theme sync to config skipped or failed:", err);
      }
    };
    if (theme) updateConfigTheme();
  }, [theme, isLockRequired]);

  const handleKeyPromptOpenChange = (open: boolean) => {
    if (!open) {
      getCurrentWindow().close();
    }
  };

  const handleUnlock = async () => {
    if (!biopassKey.trim()) {
      toast.error("Biopass key is required");
      return;
    }

    setIsValidatingKey(true);
    try {
      const valid = await cmd.system.validateConfigurationLockKey(biopassKey);
      if (valid) {
        setBiopassKey("");
        setIsLockRequired(false);
        return;
      }

      toast.error("Invalid Biopass key");
    } catch (err) {
      const message = err instanceof Error ? err.message : String(err);
      toast.error(`Failed to validate key: ${message}`);
    } finally {
      setIsValidatingKey(false);
    }
  };

  if (isCheckingLock) {
    return (
      <div className="min-h-screen bg-background text-foreground flex items-center justify-center">
        <p className="text-sm text-muted-foreground">
          Checking configuration lock...
        </p>
      </div>
    );
  }

  if (isLockRequired) {
    return (
      <Dialog open onOpenChange={handleKeyPromptOpenChange}>
        <DialogContent showCloseButton>
          <DialogHeader>
            <DialogTitle>Biopass Key Required</DialogTitle>
            <DialogDescription>
              Configuration lock is enabled. Enter your Biopass key to continue.
            </DialogDescription>
          </DialogHeader>

          <div className="grid gap-2">
            <Input
              type="password"
              value={biopassKey}
              onChange={(event) => setBiopassKey(event.target.value)}
              onKeyDown={(event) => {
                if (event.key === "Enter") {
                  event.preventDefault();
                  handleUnlock();
                }
              }}
              placeholder="Enter Biopass key"
              autoFocus
              disabled={isValidatingKey}
            />
          </div>

          <DialogFooter>
            <Button
              onClick={handleUnlock}
              disabled={isValidatingKey}
              className="cursor-pointer"
            >
              {isValidatingKey ? "Validating..." : "Unlock"}
            </Button>
          </DialogFooter>
        </DialogContent>
      </Dialog>
    );
  }

  return (
    <div className="min-h-screen bg-background text-foreground">
      {/* Navigation */}
      <nav className="sticky top-0 z-50 backdrop-blur-lg bg-background/80 border-b border-border">
        <div className="max-w-6xl mx-auto px-4">
          <div className="flex items-center justify-between h-16">
            <div className="flex items-center gap-8">
              <div className="flex items-center gap-3">
                <img src={logo} className="h-8" alt="Biopass logo" />
                <span className="font-bold text-lg hidden sm:inline-block">
                  Biopass
                </span>
              </div>

              {/* Tab Navigation */}
              <div className="flex items-center gap-2">
                <Link
                  to="/configuration"
                  className={`flex items-center gap-2 px-3 py-1.5 rounded-md transition-colors cursor-pointer ${
                    pathname === "/configuration"
                      ? "bg-primary/10 text-primary"
                      : "text-muted-foreground hover:bg-muted hover:text-foreground"
                  }`}
                >
                  <Settings className="w-4 h-4" />
                  <span className="text-sm font-medium">Configuration</span>
                </Link>
                <Link
                  to="/models"
                  className={`flex items-center gap-2 px-3 py-1.5 rounded-md transition-colors cursor-pointer ${
                    pathname === "/models"
                      ? "bg-primary/10 text-primary"
                      : "text-muted-foreground hover:bg-muted hover:text-foreground"
                  }`}
                >
                  <Cpu className="w-4 h-4" />
                  <span className="text-sm font-medium">AI Models</span>
                </Link>
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
        <Outlet />
      </main>
    </div>
  );
}

function RootLayout() {
  return (
    <>
      <App />
      <TanStackRouterDevtools />
    </>
  );
}

export const Route = createRootRoute({ component: RootLayout });
