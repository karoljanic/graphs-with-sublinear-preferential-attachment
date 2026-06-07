"""
Script to generate heatmaps showing the average number of edges to remove
to restore planarity after adding an edge (source, target) to the graph.

X-axis: source vertex index
Y-axis: target vertex index
Cell value: average min_edges_to_remove across repetitions
"""

import os
import json
import glob
import re
import argparse
from collections import defaultdict

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
from matplotlib.ticker import MaxNLocator


RESULTS_DIR = "results/planarity_restoring"
OUTPUT_DIR = "results/planarity_restoring_heatmaps"


def parse_filename(filename: str) -> dict | None:
    """Parse parameters from the filename."""
    pattern = r"n(\d+)_m(\d+)_(\w+)_a([\d.]+)_rep(\d+)_restoring\.json"
    match = re.match(pattern, filename)
    if not match:
        return None
    return {
        "n": int(match.group(1)),
        "m": int(match.group(2)),
        "fitness_type": match.group(3),
        "fitness_alpha": match.group(4),
        "rep": int(match.group(5)),
    }


def group_files_by_params(results_dir: str) -> dict:
    """Group result files by (n, m, fitness_type, fitness_alpha) parameters."""
    groups = defaultdict(list)
    for filepath in sorted(glob.glob(os.path.join(results_dir, "*.json"))):
        filename = os.path.basename(filepath)
        params = parse_filename(filename)
        if params is None:
            continue
        key = (params["n"], params["m"], params["fitness_type"], params["fitness_alpha"])
        groups[key].append(filepath)
    return groups


def compute_heatmap_data(filepaths: list, n: int) -> np.ndarray:
    """
    Compute the average min_edges_to_remove for each (source, target) pair
    across all repetitions in the given file list.

    Returns an n x n matrix where entry [i][j] is the average number of edges
    to remove when adding edge (i, j). NaN means no data for that pair.
    """
    sum_matrix = np.zeros((n, n), dtype=np.float64)
    count_matrix = np.zeros((n, n), dtype=np.int32)

    for filepath in filepaths:
        with open(filepath, "r") as f:
            data = json.load(f)

        for result in data["restoring_results"]:
            src = result["added_edge"]["source"]
            tgt = result["added_edge"]["target"]
            removal_count = result["min_edges_to_remove"]

            sum_matrix[src, tgt] += removal_count
            count_matrix[src, tgt] += 1
            # Make symmetric (edge (i,j) == edge (j,i))
            sum_matrix[tgt, src] += removal_count
            count_matrix[tgt, src] += 1

    # Compute average, use NaN where no data
    with np.errstate(divide="ignore", invalid="ignore"):
        avg_matrix = np.where(count_matrix > 0, sum_matrix / count_matrix, np.nan)

    return avg_matrix


def compute_removed_edge_index_heatmaps(filepaths: list, n: int):
    """
    Compute two heatmaps:
    - avg_min_index: average of the smaller vertex index of removed edges
    - avg_max_index: average of the larger vertex index of removed edges

    For each added edge (i, j), we look at all removal options, collect all
    removed edges, and compute the mean of min(src, tgt) and max(src, tgt).
    """
    sum_min_matrix = np.zeros((n, n), dtype=np.float64)
    sum_max_matrix = np.zeros((n, n), dtype=np.float64)
    count_matrix = np.zeros((n, n), dtype=np.int32)

    for filepath in filepaths:
        with open(filepath, "r") as f:
            data = json.load(f)

        for result in data["restoring_results"]:
            src = result["added_edge"]["source"]
            tgt = result["added_edge"]["target"]
            options = result["removal_options"]

            # Collect all removed edges from all options
            min_indices = []
            max_indices = []
            for option in options:
                for edge in option:
                    e_src = edge["source"]
                    e_tgt = edge["target"]
                    min_indices.append(min(e_src, e_tgt))
                    max_indices.append(max(e_src, e_tgt))

            if not min_indices:
                continue

            avg_min = np.mean(min_indices)
            avg_max = np.mean(max_indices)

            sum_min_matrix[src, tgt] += avg_min
            sum_min_matrix[tgt, src] += avg_min
            sum_max_matrix[src, tgt] += avg_max
            sum_max_matrix[tgt, src] += avg_max
            count_matrix[src, tgt] += 1
            count_matrix[tgt, src] += 1

    with np.errstate(divide="ignore", invalid="ignore"):
        avg_min_matrix = np.where(count_matrix > 0, sum_min_matrix / count_matrix, np.nan)
        avg_max_matrix = np.where(count_matrix > 0, sum_max_matrix / count_matrix, np.nan)

    return avg_min_matrix, avg_max_matrix


def group_matrix(avg_matrix: np.ndarray, group_size: int) -> np.ndarray:
    """Bin the heatmap matrix by grouping indices into bins of group_size.

    Each cell in the output is the mean of the corresponding group_size x group_size
    block in the input (ignoring NaN values).
    """
    n = avg_matrix.shape[0]
    num_bins = int(np.ceil(n / group_size))
    grouped = np.full((num_bins, num_bins), np.nan)

    for i in range(num_bins):
        for j in range(num_bins):
            block = avg_matrix[
                i * group_size : min((i + 1) * group_size, n),
                j * group_size : min((j + 1) * group_size, n),
            ]
            if np.any(~np.isnan(block)):
                grouped[i, j] = np.nanmean(block)

    return grouped


def plot_heatmap(avg_matrix: np.ndarray, n: int, m: int, fitness_type: str,
                 fitness_alpha: str, num_reps: int, output_path: str,
                 group_size: int = 1, title_metric: str = "średnia liczba krawędzi do usunięcia",
                 cbar_label: str = "Średnia liczba krawędzi do usunięcia",
                 cmap_name: str = "YlOrRd", fixed_vmax: float | None = None):
    """Plot and save a heatmap."""
    if group_size > 1:
        plot_data = group_matrix(avg_matrix, group_size)
    else:
        plot_data = avg_matrix

    fig, ax = plt.subplots(figsize=(10, 8))

    # Mask NaN values for better visualization
    masked_data = np.ma.masked_where(np.isnan(plot_data), plot_data)

    # Use a colormap where NaN (no data) is shown as white/light gray
    cmap = plt.cm.get_cmap(cmap_name).copy()
    cmap.set_bad(color="lightgray")

    vmin = 0
    if fixed_vmax is not None:
        vmax = fixed_vmax
    else:
        vmax = np.nanmax(plot_data) if not np.all(np.isnan(plot_data)) else 1

    im = ax.imshow(masked_data, cmap=cmap, interpolation="nearest",
                   origin="lower", vmin=vmin, vmax=vmax, aspect="equal")

    cbar = plt.colorbar(im, ax=ax, shrink=0.8)
    cbar.set_label(cbar_label, fontsize=11)

    ax.set_xlabel("Indeks wierzchołka (source)", fontsize=12)
    ax.set_ylabel("Indeks wierzchołka (target)", fontsize=12)

    group_label = f", grupowanie={group_size}" if group_size > 1 else ""
    ax.set_title(
        f"Heatmapa: {title_metric}\n"
        f"n={n}, m={m}, fitness={fitness_type}, α={fitness_alpha}, "
        f"powtórzeń={num_reps}{group_label}",
        fontsize=12,
    )

    # Set tick labels showing vertex index ranges
    num_bins = plot_data.shape[0]
    if group_size > 1:
        tick_step = max(1, num_bins // 10)
        tick_positions = range(0, num_bins, tick_step)
        tick_labels = [f"{i * group_size}" for i in tick_positions]
        ax.set_xticks(list(tick_positions))
        ax.set_yticks(list(tick_positions))
        ax.set_xticklabels(tick_labels)
        ax.set_yticklabels(tick_labels)
    else:
        tick_step = max(1, n // 10)
        ax.set_xticks(range(0, n, tick_step))
        ax.set_yticks(range(0, n, tick_step))
        ax.set_xticklabels(range(0, n, tick_step))
        ax.set_yticklabels(range(0, n, tick_step))

    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"  Saved: {output_path}")


def main():
    parser = argparse.ArgumentParser(
        description="Generate heatmaps of planarity restoring results."
    )
    parser.add_argument(
        "--group-size", "-g", type=int, default=1,
        help="Group vertex indices into bins of this size (e.g. 3 or 5). Default: 1 (no grouping)"
    )
    args = parser.parse_args()
    group_size = args.group_size

    output_dir = OUTPUT_DIR
    if group_size > 1:
        output_dir = f"{OUTPUT_DIR}_g{group_size}"

    os.makedirs(output_dir, exist_ok=True)

    groups = group_files_by_params(RESULTS_DIR)

    if not groups:
        print(f"No result files found in {RESULTS_DIR}")
        return

    print(f"Found {len(groups)} parameter configurations:")
    if group_size > 1:
        print(f"Grouping indices by {group_size}")
    for key, files in sorted(groups.items()):
        n, m, fitness_type, fitness_alpha = key
        print(f"  n={n}, m={m}, fitness={fitness_type}, α={fitness_alpha}: "
              f"{len(files)} repetitions")

    print("\nGenerating heatmaps...")
    for key, filepaths in sorted(groups.items()):
        n, m, fitness_type, fitness_alpha = key
        print(f"\nProcessing: n={n}, m={m}, fitness={fitness_type}, α={fitness_alpha}")

        suffix = f"_g{group_size}" if group_size > 1 else ""

        # --- Heatmap 1: average number of edges to remove ---
        avg_matrix = compute_heatmap_data(filepaths, n)
        valid_count = np.count_nonzero(~np.isnan(avg_matrix))
        if valid_count > 0:
            print(f"  [removal count] Valid cells: {valid_count}/{n*n}, "
                  f"mean={np.nanmean(avg_matrix):.3f}, "
                  f"max={np.nanmax(avg_matrix):.1f}")

        output_filename = f"heatmap_n{n}_m{m}_{fitness_type}_a{fitness_alpha}{suffix}.png"
        output_path = os.path.join(output_dir, output_filename)
        plot_heatmap(avg_matrix, n, m, fitness_type, fitness_alpha,
                     len(filepaths), output_path, group_size=group_size,
                     fixed_vmax=3.0)

        # --- Heatmaps 2 & 3: min/max vertex index of removed edges ---
        avg_min_idx, avg_max_idx = compute_removed_edge_index_heatmaps(filepaths, n)

        valid_min = np.count_nonzero(~np.isnan(avg_min_idx))
        if valid_min > 0:
            print(f"  [min idx removed] mean={np.nanmean(avg_min_idx):.2f}, "
                  f"max={np.nanmax(avg_min_idx):.1f}")

        output_filename_min = f"heatmap_min_idx_n{n}_m{m}_{fitness_type}_a{fitness_alpha}{suffix}.png"
        output_path_min = os.path.join(output_dir, output_filename_min)
        plot_heatmap(avg_min_idx, n, m, fitness_type, fitness_alpha,
                     len(filepaths), output_path_min, group_size=group_size,
                     title_metric="śr. mniejszy indeks wierzchołka usuniętej krawędzi",
                     cbar_label="Średni mniejszy indeks wierzchołka",
                     cmap_name="YlOrRd", fixed_vmax=float(n))

        valid_max = np.count_nonzero(~np.isnan(avg_max_idx))
        if valid_max > 0:
            print(f"  [max idx removed] mean={np.nanmean(avg_max_idx):.2f}, "
                  f"max={np.nanmax(avg_max_idx):.1f}")

        output_filename_max = f"heatmap_max_idx_n{n}_m{m}_{fitness_type}_a{fitness_alpha}{suffix}.png"
        output_path_max = os.path.join(output_dir, output_filename_max)
        plot_heatmap(avg_max_idx, n, m, fitness_type, fitness_alpha,
                     len(filepaths), output_path_max, group_size=group_size,
                     title_metric="śr. większy indeks wierzchołka usuniętej krawędzi",
                     cbar_label="Średni większy indeks wierzchołka",
                     cmap_name="YlOrRd", fixed_vmax=float(n))

    print("\nDone! All heatmaps saved to:", output_dir)


if __name__ == "__main__":
    main()
