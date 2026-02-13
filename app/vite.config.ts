import path from "node:path";
import tailwindcss from "@tailwindcss/vite";
import react from "@vitejs/plugin-react";
import { defineConfig } from "vite";

const host = process.env.TAURI_DEV_HOST;

// https://vite.dev/config/
export default defineConfig(async () => ({
  plugins: [react(), tailwindcss()],
  resolve: {
    alias: {
      "@": path.resolve(__dirname, "./src"),
    },
  },

  build: {
    rollupOptions: {
      output: {
        manualChunks: (id) => {
          if (id.includes("node_modules")) {
            // Group Tauri-related packages (usually independent)
            if (id.includes("@tauri-apps")) {
              return "vendor-tauri";
            }
            // Group Dnd-kit (can be large and is relatively independent)
            if (id.includes("@dnd-kit")) {
              return "vendor-dnd";
            }
            // Put all other node_modules (including React) into a single vendor chunk
            // to avoid circular dependency cycles between libraries.
            return "vendor";
          }
          // Group configuration UI components for better organization
          if (id.includes("src/components/config")) {
            return "config-ui";
          }
        },
      },
    },
  },

  // Vite options tailored for Tauri development and only applied in `tauri dev` or `tauri build`
  //
  // 1. prevent Vite from obscuring rust errors
  clearScreen: false,
  // 2. tauri expects a fixed port, fail if that port is not available
  server: {
    port: 1420,
    strictPort: true,
    host: host || false,
    hmr: host
      ? {
          protocol: "ws",
          host,
          port: 1421,
        }
      : undefined,
    watch: {
      // 3. tell Vite to ignore watching `src-tauri`
      ignored: ["**/src-tauri/**"],
    },
  },
}));
