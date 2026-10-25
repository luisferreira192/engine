from pathlib import Path
import re

import pandas as pd
import matplotlib.pyplot as plt


ROOT = Path("timings/2026-09-26")
IGNORE_DIRS = {"22-28"}


# ============================================================
# FILE LOADING
# ============================================================

def get_run_params(path: Path):
    """
    Example:
        timelog_1000_49_2026-09-26_22-32-57.csv

        element_count = 1000
        cell_n        = 49
    """

    match = re.search(
        r"_(\d+)_(\d+)_\d{4}-\d{2}-\d{2}_",
        path.name
    )

    if not match:
        raise ValueError(
            f"Could not extract element_count/cell_n from {path}"
        )

    element_count = int(match.group(1))
    cell_n = int(match.group(2))

    return element_count, cell_n


def load_files(prefix: str) -> pd.DataFrame:
    dfs = []

    for path in ROOT.rglob(f"{prefix}_*.csv"):

        if any(part in IGNORE_DIRS for part in path.parts):
            continue

        element_count, cell_n = get_run_params(path)

        df = pd.read_csv(path)

        df["element_count"] = element_count
        df["cell_n"] = cell_n
        df["run_dir"] = path.parent.name
        df["source_file"] = path.name

        dfs.append(df)

    if not dfs:
        raise RuntimeError(f"No files found for {prefix}")

    return pd.concat(dfs, ignore_index=True)


# ============================================================
# LOAD DATA
# ============================================================

timings = load_files("timelog")
candidates = load_files("candidatestats")


# Each line corresponds to another 120-frame sample.
timings["sample"] = (
    timings
    .groupby(["element_count", "cell_n", "metric"])
    .cumcount()
)

candidates["sample"] = (
    candidates
    .groupby(["element_count", "cell_n"])
    .cumcount()
)

timings = timings.sort_values(
    ["element_count", "cell_n", "sample"]
)

candidates = candidates.sort_values(
    ["element_count", "cell_n", "sample"]
)


print("\nTiming runs:")
print(
    timings[
        [
            "element_count",
            "cell_n",
            "run_dir",
            "source_file",
        ]
    ]
    .drop_duplicates()
    .sort_values(["element_count", "cell_n"])
)

print("\nCandidate runs:")
print(
    candidates[
        [
            "element_count",
            "cell_n",
            "run_dir",
            "source_file",
        ]
    ]
    .drop_duplicates()
    .sort_values(["element_count", "cell_n"])
)


# ============================================================
# TIMING ANALYSIS
# ============================================================

timing_summary = (
    timings
    .groupby(["element_count", "cell_n", "metric"])
    .agg(
        mean_avg_us=("avg_us", "mean"),
        median_avg_us=("avg_us", "median"),
        mean_min_us=("min_us", "mean"),
        mean_max_us=("max_us", "mean"),
        worst_us=("max_us", "max"),
    )
    .reset_index()
)


# ------------------------------------------------------------
# Frame time
# ------------------------------------------------------------

frame_stats = timings[
    timings["metric"] == "frame"
]

frame_summary = (
    frame_stats
    .groupby(["element_count", "cell_n"])
    .agg(
        avg_frame_us=("avg_us", "mean"),
        median_frame_us=("avg_us", "median"),
        worst_frame_us=("max_us", "max"),
    )
)

frame_summary["avg_fps"] = (
    1_000_000 / frame_summary["avg_frame_us"]
)


# ------------------------------------------------------------
# Collision time
# ------------------------------------------------------------

collision_stats = timings[
    timings["metric"] == "collision"
]

collision_summary = (
    collision_stats
    .groupby(["element_count", "cell_n"])
    .agg(
        avg_collision_us=("avg_us", "mean"),
        median_collision_us=("avg_us", "median"),
        worst_collision_us=("max_us", "max"),
    )
)


# ============================================================
# CANDIDATE ANALYSIS
# ============================================================

candidate_columns = [
    "candidates",
    "duplicates",
    "unique_candidates",
    "actual_collisions",
]


# ------------------------------------------------------------
# Cumulative final values
# ------------------------------------------------------------

candidate_final = (
    candidates
    .sort_values("sample")
    .groupby(["element_count", "cell_n"])
    .tail(1)
    .set_index(["element_count", "cell_n"])
    .sort_index()
)

candidate_final["duplicate_ratio"] = (
    candidate_final["duplicates"]
    / candidate_final["candidates"]
)

candidate_final["collision_ratio"] = (
    candidate_final["actual_collisions"]
    / candidate_final["unique_candidates"]
)


# ------------------------------------------------------------
# Work performed per 120-frame sample
# ------------------------------------------------------------

for column in candidate_columns:
    candidates[column + "_delta"] = (
        candidates
        .groupby(["element_count", "cell_n"])[column]
        .diff()
    )


delta_summary = (
    candidates
    .groupby(["element_count", "cell_n"])
    .agg(
        candidates_per_sample=("candidates_delta", "mean"),
        duplicates_per_sample=("duplicates_delta", "mean"),
        unique_per_sample=("unique_candidates_delta", "mean"),
        collisions_per_sample=("actual_collisions_delta", "mean"),
    )
)


# ============================================================
# COMPLETE SUMMARY
# ============================================================

summary = (
    frame_summary
    .join(collision_summary)
    .join(
        candidate_final[
            [
                "candidates",
                "duplicates",
                "unique_candidates",
                "actual_collisions",
                "duplicate_ratio",
                "collision_ratio",
            ]
        ]
    )
    .join(delta_summary)
)

print("\n================ COMPLETE SUMMARY ================\n")
print(summary.to_string())

summary.to_csv(ROOT / "analysis_summary.csv")


# ============================================================
# PLOTTING HELPER
# ============================================================

def plot_by_element_count(
    dataframe,
    column,
    ylabel,
    title,
    filename,
):
    plt.figure()

    for element_count, group in dataframe.groupby(
        level="element_count"
    ):
        group = group.sort_index(
            level="cell_n"
        )

        cell_counts = (
            group.index
            .get_level_values("cell_n")
        )

        plt.plot(
            cell_counts,
            group[column],
            marker="o",
            label=f"{element_count} elements",
        )

    plt.xlabel("Number of grid cells")
    plt.ylabel(ylabel)
    plt.title(title)

    plt.legend(title="Object count")
    plt.grid()

    plt.savefig(
        ROOT / filename,
        dpi=150,
        bbox_inches="tight"
    )

    # plt.show()


# ============================================================
# PLOTS
# ============================================================

# ------------------------------------------------------------
# 1. Frame time
# ------------------------------------------------------------

plot_by_element_count(
    frame_summary,
    "avg_frame_us",
    "Average frame time (us)",
    "Frame Time vs Grid Cell Count",
    "frame_time_vs_cells.png",
)


# ------------------------------------------------------------
# 2. FPS
# ------------------------------------------------------------

plot_by_element_count(
    frame_summary,
    "avg_fps",
    "Average FPS",
    "FPS vs Grid Cell Count",
    "fps_vs_cells.png",
)


# ------------------------------------------------------------
# 3. Collision time
# ------------------------------------------------------------

plot_by_element_count(
    collision_summary,
    "avg_collision_us",
    "Average collision time (us)",
    "Collision Time vs Grid Cell Count",
    "collision_time_vs_cells.png",
)


# ------------------------------------------------------------
# 4. Candidate pairs
# ------------------------------------------------------------

plot_by_element_count(
    delta_summary,
    "candidates_per_sample",
    "Candidate pairs per 120 frames",
    "Broad-Phase Candidates vs Grid Cell Count",
    "candidates_vs_cells.png",
)


# ------------------------------------------------------------
# 5. Unique candidate pairs
# ------------------------------------------------------------

plot_by_element_count(
    delta_summary,
    "unique_per_sample",
    "Unique candidate pairs per 120 frames",
    "Unique Candidates vs Grid Cell Count",
    "unique_candidates_vs_cells.png",
)


# ------------------------------------------------------------
# 6. Duplicate candidates
# ------------------------------------------------------------

plot_by_element_count(
    delta_summary,
    "duplicates_per_sample",
    "Duplicate pairs per 120 frames",
    "Duplicate Candidates vs Grid Cell Count",
    "duplicates_vs_cells.png",
)


# ------------------------------------------------------------
# 7. Duplicate ratio
# ------------------------------------------------------------

candidate_ratio = candidate_final[
    ["duplicate_ratio"]
].copy()

candidate_ratio["duplicate_percent"] = (
    candidate_ratio["duplicate_ratio"] * 100
)

plot_by_element_count(
    candidate_ratio,
    "duplicate_percent",
    "Duplicate candidates (%)",
    "Duplicate Ratio vs Grid Cell Count",
    "duplicate_ratio_vs_cells.png",
)


# ------------------------------------------------------------
# 8. Actual collision ratio
# ------------------------------------------------------------

collision_ratio = candidate_final[
    ["collision_ratio"]
].copy()

collision_ratio["collision_percent"] = (
    collision_ratio["collision_ratio"] * 100
)

plot_by_element_count(
    collision_ratio,
    "collision_percent",
    "Actual collisions / unique candidates (%)",
    "Collision Hit Rate vs Grid Cell Count",
    "collision_ratio_vs_cells.png",
)


# ============================================================
# COLLISION PERFORMANCE OVER TIME
# ============================================================

plt.figure()

for (element_count, cell_n), group in collision_stats.groupby(
    ["element_count", "cell_n"]
):
    plt.plot(
        group["sample"],
        group["avg_us"],
        label=f"{element_count} obj / {cell_n} cells",
    )

plt.xlabel("120-frame sample")
plt.ylabel("Collision time (us)")
plt.title("Collision Time During Each Run")

plt.legend()
plt.grid()

plt.savefig(
    ROOT / "collision_time_over_run.png",
    dpi=150,
    bbox_inches="tight"
)

# plt.show()


# ============================================================
# TIMING COMPONENT BREAKDOWN
# ============================================================

for element_count, element_data in timing_summary.groupby(
    "element_count"
):
    pivot = element_data.pivot(
        index="cell_n",
        columns="metric",
        values="mean_avg_us",
    )

    print(
        f"\n========= {element_count} ELEMENTS =========\n"
    )
    print(pivot)

    pivot.to_csv(
        ROOT
        / f"timing_components_{element_count}_elements.csv"
    )

    plt.figure()

    for metric in pivot.columns:
        plt.plot(
            pivot.index,
            pivot[metric],
            marker="o",
            label=metric,
        )

    plt.xlabel("Number of grid cells")
    plt.ylabel("Average time (us)")
    plt.title(
        f"Timing Components — {element_count} Elements"
    )

    plt.legend()
    plt.grid()

    plt.savefig(
        ROOT
        / f"timing_components_{element_count}_elements.png",
        dpi=150,
        bbox_inches="tight",
    )

   # plt.show()