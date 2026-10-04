from __future__ import annotations

import csv
import math
from pathlib import Path


INPUT_PATH = Path("theor_max_in.csv")
OUTPUT_PATH = Path("theor_max_out.csv")
LEFT_X = -0.5
RIGHT_X = 0.5
BOTTOM_Y = -0.5
TOP_Y = 0.5
SIDE = RIGHT_X - LEFT_X  # 1.0
EPS = 1e-12


def fold_to_segment(value: float, left: float, right: float) -> float:
    """Fold an unfolded coordinate into [left, right] with mirror reflections."""
    length = right - left
    shifted = (value - left) / length
    period = 2.0
    mod = math.fmod(shifted, period)
    if mod < 0.0:
        mod += period
    folded = mod if mod <= 1.0 else 2.0 - mod
    return left + folded * length


def time_to_left_wall(x0: float, vx: float) -> float:
    """Return first non-negative time when x(t) reaches LEFT_X."""
    if abs(x0 - LEFT_X) <= EPS:
        return 0.0

    if abs(vx) <= EPS:
        raise ValueError("Encountered vx=0 away from x=-0.5; input contract violated.")

    if vx < 0.0:
        # Moving directly to the left wall.
        return (x0 - LEFT_X) / (-vx)

    # vx > 0: first reach right wall, then cross full width back to the left wall.
    return (RIGHT_X - x0) / vx + SIDE / vx


def compute_y_at_left_wall(x0: float, y0: float, vx: float, vy: float) -> float:
    hit_time = time_to_left_wall(x0, vx)
    unfolded_y = y0 + vy * hit_time
    return fold_to_segment(unfolded_y, BOTTOM_Y, TOP_Y)


def parse_row(row: list[str], line_number: int) -> tuple[float, float, float, float]:
    if len(row) != 4:
        raise ValueError(f"Line {line_number}: expected 4 comma-separated values, got {len(row)}.")

    try:
        x0, y0, vx, vy = (float(value.strip()) for value in row)
    except ValueError as exc:
        raise ValueError(f"Line {line_number}: failed to parse float values.") from exc

    return x0, y0, vx, vy


def main() -> None:
    with INPUT_PATH.open("r", newline="", encoding="utf-8") as infile, OUTPUT_PATH.open(
        "w", newline="", encoding="utf-8"
    ) as outfile:
        reader = csv.reader(infile)
        writer = csv.writer(outfile, lineterminator="\n")

        for line_number, row in enumerate(reader, start=1):
            if not row:
                raise ValueError(f"Line {line_number}: empty line is not allowed.")

            x0, y0, vx, vy = parse_row(row, line_number)
            y_hit = compute_y_at_left_wall(x0, y0, vx, vy)
            writer.writerow([str(float(y_hit))])


if __name__ == "__main__":
    main()
