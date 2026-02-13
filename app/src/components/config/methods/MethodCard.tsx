import { ChevronDown } from "lucide-react";
import type React from "react";
import { Badge } from "@/components/ui/badge";
import { Switch } from "@/components/ui/switch";
import { cn } from "@/lib/utils";

interface Props {
  title: string;
  icon: React.ReactNode;
  color: string;
  enabled: boolean;
  onToggle: (enabled: boolean) => void;
  expanded: boolean;
  onExpand: () => void;
  children: React.ReactNode;
}

export function MethodCard({
  title,
  icon,
  color,
  enabled,
  onToggle,
  expanded,
  onExpand,
  children,
}: Props) {
  return (
    <div
      className={cn(
        "group relative overflow-hidden rounded-xl border transition-all duration-200",
        expanded
          ? "border-border bg-muted/30 shadow-md ring-2 ring-primary/20"
          : "border-border bg-background/50 hover:bg-muted/20 shadow-xs",
      )}
    >
      <div className="p-4 flex items-center justify-between gap-4">
        <div
          className="flex items-center gap-4 flex-1 cursor-pointer group/header"
          onClick={onExpand}
          onKeyDown={(e) => e.key === "Enter" && onExpand()}
          role="button"
          tabIndex={0}
        >
          <div
            className={cn(
              "w-10 h-10 rounded-lg bg-linear-to-br flex items-center justify-center transition-transform group-hover/header:scale-110 shadow-sm",
              color,
            )}
          >
            {icon}
          </div>
          <div className="flex-1">
            <h3 className="font-medium text-sm sm:text-base">{title}</h3>
            {!expanded && (
              <Badge
                variant={enabled ? "default" : "secondary"}
                className={cn(
                  "mt-1 text-[10px] h-4 px-1.5 transition-colors",
                  enabled
                    ? "bg-emerald-500/10 text-emerald-500 border-emerald-500/20"
                    : "bg-muted text-muted-foreground",
                )}
              >
                {enabled ? "Enabled" : "Disabled"}
              </Badge>
            )}
          </div>
        </div>

        <div
          className="flex items-center gap-4"
          onClick={(e) => e.stopPropagation()}
          onPointerDown={(e) => e.stopPropagation()}
          onMouseDown={(e) => e.stopPropagation()}
          onKeyDown={(e) => e.stopPropagation()}
        >
          <div className="flex items-center gap-2 pr-2 border-r border-border/50">
            <Switch
              checked={enabled}
              onCheckedChange={onToggle}
              className="cursor-pointer"
            />
          </div>
          <button
            type="button"
            onClick={(e) => {
              e.stopPropagation();
              onExpand();
            }}
            className={cn(
              "w-8 h-8 rounded-lg flex items-center justify-center hover:bg-muted/80 transition-all cursor-pointer",
              expanded && "bg-muted/80 rotate-180",
            )}
          >
            <ChevronDown className="w-4 h-4" />
          </button>
        </div>
      </div>

      <div
        className={cn(
          "grid transition-all duration-200 ease-in-out",
          expanded
            ? "grid-rows-[1fr] opacity-100"
            : "grid-rows-[0fr] opacity-0",
        )}
      >
        <div className="overflow-hidden">
          <div className="px-4 pb-4">{children}</div>
        </div>
      </div>
    </div>
  );
}
