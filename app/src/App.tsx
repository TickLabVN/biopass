import { invoke } from "@tauri-apps/api/core";
import { User } from "lucide-react";
import { useEffect, useState } from "react";
import { ConfigPage } from "@/components/config";
import logo from "./assets/logo.png";

function App() {
  const [username, setUsername] = useState("");

  useEffect(() => {
    invoke<string>("get_current_username")
      .then(setUsername)
      .catch((err) => console.error("Failed to get username:", err));
  }, []);

  return (
    <div className="min-h-screen bg-background text-foreground">
      {/* Navigation */}
      <nav className="sticky top-0 z-50 backdrop-blur-lg bg-background/80 border-b border-border">
        <div className="max-w-6xl mx-auto px-4">
          <div className="flex items-center justify-between h-16">
            <div className="flex items-center gap-3">
              <img src={logo} className="h-8" alt="Facepass logo" />
              <span className="font-bold text-lg">Facepass</span>
            </div>

            {username && (
              <div className="flex items-center gap-2 px-3 py-1.5 rounded-full bg-muted/50 border border-border/50">
                <User className="w-4 h-4 text-muted-foreground" />
                <span className="text-sm font-medium">{username}</span>
              </div>
            )}
          </div>
        </div>
      </nav>

      {/* Content */}
      <main className="max-w-6xl mx-auto">
        <ConfigPage />
      </main>
    </div>
  );
}

export default App;
