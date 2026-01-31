import { invoke } from "@tauri-apps/api/core";
import { useState } from "react";
import { CameraSample } from "@/components/samples/CameraSample";
import { VoiceSample } from "@/components/samples/VoiceSample";
import { Button } from "@/components/ui/button";
import logo from "./assets/logo.png";

function App() {
  const [greetMsg, setGreetMsg] = useState("");
  const [name, setName] = useState("");
  const [showSamples, setShowSamples] = useState(false);

  async function greet() {
    // Learn more about Tauri commands at https://tauri.app/develop/calling-rust/
    setGreetMsg(await invoke("greet", { name }));
  }

  return (
    <main className="flex flex-col items-center justify-center min-h-screen p-8 bg-background text-foreground text-center">
      <img
        src={logo}
        className="h-32 mb-8 hover:scale-105 transition-transform"
        alt="Facepass logo"
      />
      <h1 className="text-4xl font-bold mb-4">Facepass</h1>
      <p className="mb-8 text-muted-foreground max-w-md">
        Modern FaceID login for Linux. A fast, secure, and privacy-focused
        facial recognition module.
      </p>

      <div className="flex flex-col gap-8 w-full max-w-2xl items-center">
        {!showSamples ? (
          <div className="flex flex-col items-center gap-4">
            <form
              className="flex gap-2 mb-4"
              onSubmit={(e) => {
                e.preventDefault();
                greet();
              }}
            >
              <input
                id="greet-input"
                className="flex h-10 w-full rounded-md border border-input bg-background px-3 py-2 text-sm ring-offset-background file:border-0 file:bg-transparent file:text-sm file:font-medium placeholder:text-muted-foreground focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring focus-visible:ring-offset-2 disabled:cursor-not-allowed disabled:opacity-50"
                onChange={(e) => setName(e.currentTarget.value)}
                placeholder="Enter a name..."
              />
              <Button type="submit">Greet</Button>
            </form>
            <p className="font-medium h-6">{greetMsg}</p>
            <Button variant="outline" onClick={() => setShowSamples(true)}>
              View Input Samples
            </Button>
          </div>
        ) : (
          <div className="flex flex-col gap-6 w-full animate-in fade-in slide-in-from-bottom-4 duration-500">
            <div className="flex justify-between items-center mb-2">
              <h2 className="text-2xl font-bold">Input Samples</h2>
              <Button variant="ghost" onClick={() => setShowSamples(false)}>
                Back to Greet
              </Button>
            </div>
            <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
              <CameraSample />
              <VoiceSample />
            </div>
          </div>
        )}
      </div>
    </main>
  );
}

export default App;
