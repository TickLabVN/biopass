import { Label } from "@/components/ui/label";
import { Slider } from "@/components/ui/slider";

interface Props {
  label: string;
  value: number;
  onChange: (value: number) => void;
}

export function SliderField({ label, value, onChange }: Props) {
  return (
    <div className="grid gap-2">
      <div className="flex items-center justify-between">
        <Label className="text-xs text-muted-foreground">{label}</Label>
        <span className="text-xs font-mono font-medium">
          {(value * 100).toFixed(0)}%
        </span>
      </div>
      <div className="h-9 flex items-center">
        <Slider
          value={[value]}
          max={1}
          step={0.01}
          onValueChange={([v]) => onChange(v)}
          className="cursor-pointer"
        />
      </div>
    </div>
  );
}
