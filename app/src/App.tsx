import { ConfigPage } from "@/components/config";
import logo from "./assets/logo.png";

function App() {
  return (
    <div className="min-h-screen bg-background text-foreground">
      {/* Navigation */}
      <nav className="sticky top-0 z-50 backdrop-blur-lg bg-background/80 border-b border-border">
        <div className="max-w-6xl mx-auto px-4">
          <div className="flex items-center h-16">
            <div className="flex items-center gap-3">
              <img src={logo} className="h-8" alt="Facepass logo" />
              <span className="font-bold text-lg">Facepass</span>
            </div>
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
