import { AlertCircle, Loader2 } from "lucide-react";
import { Badge } from "@/components/ui/badge";

export type ModelStatusType =
  | "checking"
  | "available"
  | "missing"
  | "inuse"
  | "downloading";

interface ModelStatusProps {
  status?: ModelStatusType | boolean;
  progress?: number;
  className?: string;
  size?: "sm" | "default";
}

export function ModelStatus({
  status,
  progress = 0,
  className,
  size = "default",
}: ModelStatusProps) {
  const isSmall = size === "sm";
  const badgeClass = isSmall ? "text-[10px] px-1.5 h-5 gap-1" : "gap-1.5 h-6";
  const iconSize = isSmall ? 12 : 14;
  const dotClass = isSmall ? "w-1 h-1" : "w-1.5 h-1.5";

  if (status === "checking" || status === undefined) {
    return (
      <Badge variant="secondary" className={`${badgeClass} ${className}`}>
        <Loader2 size={iconSize} className="animate-spin" />
        {isSmall ? "Wait" : "Checking..."}
      </Badge>
    );
  }

  if (status === "downloading") {
    return (
      <Badge
        className={`bg-amber-500/10 text-amber-600 hover:bg-amber-500/20 dark:text-amber-400 border-amber-200 dark:border-amber-800 ${badgeClass} ${className}`}
      >
        <Loader2 size={iconSize} className="animate-spin" />
        {isSmall
          ? `${Math.round(progress)}%`
          : `Downloading ${Math.round(progress)}%`}
      </Badge>
    );
  }

  if (status === "inuse") {
    return (
      <Badge
        className={`bg-blue-500/10 text-blue-600 hover:bg-blue-500/20 dark:text-blue-400 border-blue-200 dark:border-blue-800 ${badgeClass} ${className}`}
      >
        <div className={`${dotClass} rounded-full bg-blue-500 animate-pulse`} />
        In Use
      </Badge>
    );
  }

  if (status === "available" || status === true) {
    return (
      <Badge
        className={`bg-emerald-500/10 text-emerald-600 hover:bg-emerald-500/20 dark:text-emerald-400 border-emerald-200 dark:border-emerald-800 ${badgeClass} ${className}`}
      >
        <div className={`${dotClass} rounded-full bg-emerald-500`} />
        {isSmall ? "Ready" : "Available"}
      </Badge>
    );
  }

  // Missing or false
  return (
    <Badge
      variant="destructive"
      className={`bg-red-500/10 text-red-600 hover:bg-red-500/20 border-red-200 dark:text-red-400 dark:border-red-900/50 dark:bg-red-500/10 ${badgeClass} ${className}`}
    >
      <AlertCircle size={iconSize} className="dark:text-red-400" />
      Missing
    </Badge>
  );
}
