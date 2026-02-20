import {
  closestCenter,
  DndContext,
  type DragEndEvent,
  KeyboardSensor,
  PointerSensor,
  useSensor,
  useSensors,
} from "@dnd-kit/core";
import {
  arrayMove,
  SortableContext,
  sortableKeyboardCoordinates,
  useSortable,
  verticalListSortingStrategy,
} from "@dnd-kit/sortable";
import { CSS } from "@dnd-kit/utilities";
import { GripVertical, Zap } from "lucide-react";
import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select";
import { Switch } from "@/components/ui/switch";
import type { StrategyConfig } from "@/types/config";

interface Props {
  strategy: StrategyConfig;
  onChange: (strategy: StrategyConfig) => void;
}

export function StrategySection({ strategy, onChange }: Props) {
  const sensors = useSensors(
    useSensor(PointerSensor),
    useSensor(KeyboardSensor, {
      coordinateGetter: sortableKeyboardCoordinates,
    }),
  );

  function handleDragEnd(event: DragEndEvent) {
    const { active, over } = event;

    if (over && active.id !== over.id) {
      const oldIndex = strategy.order.indexOf(active.id as string);
      const newIndex = strategy.order.indexOf(over.id as string);
      const newOrder = arrayMove(strategy.order, oldIndex, newIndex);
      onChange({ ...strategy, order: newOrder });
    }
  }

  return (
    <div className="rounded-xl border border-border/50 bg-card/50 backdrop-blur-sm p-6 shadow-lg">
      <h2 className="text-xl font-semibold mb-4 flex items-center gap-2">
        <span className="w-8 h-8 rounded-lg bg-linear-to-br from-blue-500 to-cyan-500 flex items-center justify-center">
          <Zap className="w-4 h-4 text-white" />
        </span>
        Strategy Settings
      </h2>

      <div className="grid gap-6">
        {/* PAM Enablement */}
        <div className="flex items-center justify-between p-4 rounded-lg bg-primary/5 border border-primary/10 transition-all">
          <div className="grid gap-0.5">
            <Label
              htmlFor="pam-enabled"
              className="text-sm font-semibold flex items-center gap-2"
            >
              System Sign-in Integration
              <span className="text-[10px] px-1.5 py-0.5 rounded-full bg-primary/20 text-primary font-bold uppercase tracking-wider">
                Recommended
              </span>
            </Label>
            <p className="text-xs text-muted-foreground max-w-[400px]">
              Use FacePass to unlock your computer and authorize system actions.
            </p>
          </div>
          <Switch
            id="pam-enabled"
            checked={strategy.pam_enabled}
            onCheckedChange={(checked) =>
              onChange({ ...strategy, pam_enabled: checked })
            }
          />
        </div>

        {/* Debug Logging Toggle */}
        <div className="flex items-center justify-between p-3 rounded-lg border border-border transition-all">
          <div className="grid gap-0.5">
            <Label
              htmlFor="debug-enabled"
              className="text-sm font-medium flex items-center gap-2"
            >
              Verbose Debug Logging
            </Label>
            <p className="text-xs text-muted-foreground max-w-[400px]">
              Enable detailed console output for authentication methods. Useful
              for troubleshooting.
            </p>
          </div>
          <Switch
            id="debug-enabled"
            checked={strategy.debug}
            onCheckedChange={(checked) =>
              onChange({ ...strategy, debug: checked })
            }
          />
        </div>

        {/* Execution Mode */}
        <div className="grid gap-2.5">
          <Label className="text-sm font-medium text-muted-foreground">
            Execution Mode
          </Label>
          <Select
            value={strategy.execution_mode}
            onValueChange={(value) =>
              onChange({
                ...strategy,
                execution_mode: value as "sequential" | "parallel",
              })
            }
          >
            <SelectTrigger className="w-full h-10 transition-all">
              <SelectValue placeholder="Select execution mode" />
            </SelectTrigger>
            <SelectContent position="popper">
              <SelectItem value="sequential" className="cursor-pointer">
                Sequential
              </SelectItem>
              <SelectItem value="parallel" className="cursor-pointer">
                Parallel
              </SelectItem>
            </SelectContent>
          </Select>
          <p className="text-xs text-muted-foreground">
            {strategy.execution_mode === "sequential"
              ? "Methods are tried in order until one succeeds"
              : "All methods run simultaneously, first success wins"}
          </p>
        </div>

        {/* Method Order - Only show in sequential mode */}
        {strategy.execution_mode === "sequential" && (
          <div className="grid gap-2.5">
            <Label className="text-sm font-medium text-muted-foreground">
              Method Priority Order
              <span className="text-xs text-muted-foreground/70 ml-2">
                (drag to reorder)
              </span>
            </Label>
            <DndContext
              sensors={sensors}
              collisionDetection={closestCenter}
              onDragEnd={handleDragEnd}
            >
              <SortableContext
                items={strategy.order}
                strategy={verticalListSortingStrategy}
              >
                <div className="flex flex-col gap-2">
                  {strategy.order.map((method, index) => (
                    <SortableMethodItem
                      key={method}
                      id={method}
                      index={index}
                    />
                  ))}
                </div>
              </SortableContext>
            </DndContext>
          </div>
        )}

        {/* Retries and Delay */}
        <div className="grid grid-cols-2 gap-6 mt-2">
          <div className="grid gap-2">
            <Label
              htmlFor="max-retries"
              className="text-sm font-medium text-muted-foreground"
            >
              Max Retries
            </Label>
            <Input
              id="max-retries"
              type="number"
              min="0"
              max="10"
              value={strategy.retries}
              onChange={(e) =>
                onChange({
                  ...strategy,
                  retries: parseInt(e.target.value, 10) || 0,
                })
              }
              className="h-10"
            />
          </div>
          <div className="grid gap-2">
            <Label
              htmlFor="retry-delay"
              className="text-sm font-medium text-muted-foreground"
            >
              Retry Delay (ms)
            </Label>
            <Input
              id="retry-delay"
              type="number"
              min="0"
              max="5000"
              step="100"
              value={strategy.retry_delay}
              onChange={(e) =>
                onChange({
                  ...strategy,
                  retry_delay: parseInt(e.target.value, 10) || 0,
                })
              }
              className="h-10"
            />
          </div>
        </div>
      </div>
    </div>
  );
}

function SortableMethodItem({ id, index }: { id: string; index: number }) {
  const {
    attributes,
    listeners,
    setNodeRef,
    transform,
    transition,
    isDragging,
  } = useSortable({ id });

  const style = {
    transform: CSS.Transform.toString(transform),
    transition,
  };

  return (
    <div
      ref={setNodeRef}
      style={style}
      className={`flex items-center gap-2 p-3 rounded-lg bg-background border border-border transition-shadow ${
        isDragging ? "shadow-lg ring-2 ring-primary/50 z-50" : ""
      }`}
    >
      <button
        type="button"
        className="cursor-grab active:cursor-grabbing p-1 rounded hover:bg-muted text-muted-foreground"
        {...attributes}
        {...listeners}
      >
        <GripVertical className="w-4 h-4" />
      </button>
      <span className="w-6 h-6 rounded-full bg-muted flex items-center justify-center text-xs font-bold">
        {index + 1}
      </span>
      <span className="flex-1 capitalize font-medium">{id}</span>
    </div>
  );
}
