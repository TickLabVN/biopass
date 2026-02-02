import React from "react";
import ReactDOM from "react-dom/client";
import { ThemeProvider } from "@/components/theme-provider";
import { Toaster } from "@/components/ui/sonner";
import App from "./App";
import "./index.css";

ReactDOM.createRoot(document.getElementById("root") as HTMLElement).render(
  <React.StrictMode>
    <ThemeProvider
      attribute="class"
      defaultTheme="system"
      enableSystem
      enableColorScheme
    >
      <App />
      <Toaster position="bottom-right" richColors />
    </ThemeProvider>
  </React.StrictMode>,
);
