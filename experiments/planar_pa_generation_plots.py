"""
Visualizations for planar PA generation experiments:
1. Boxplot: number of edges in final graph by strategy
3. Line plot: cumulative edge count evolution over steps
5. Degree distribution comparison between strategies
"""

import os
import json
import glob
import re
from collections import defaultdict

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator


RESULTS_DIR = "results/planar_pa_generation"
OUTPUT_DIR = "results/planar_pa_generation_plots"

STRATEGY_LABELS = {
    "RANDOM": "Losowa",
    "MAX_SUM_OF_MAX_EDGE_INDICES": "Max(suma max indeksów)",
    "MAX_SUM_OF_MIN_EDGE_INDICES": "Max(suma min indeksów)",
    "MIN_SUM_OF_MAX_EDGE_INDICES": "Min(suma max indeksów)",
    "MIN_SUM_OF_MIN_EDGE_INDICES": "Min(suma min indeksów)",
}

STRATEGY_COLORS = {
    "RANDOM": "#1f77b4",
    "MAX_SUM_OF_MAX_EDGE_INDICES": "#ff7f0e",
    "MAX_SUM_OF_MIN_EDGE_INDICES": "#2ca02c",
    "MIN_SUM_OF_MAX_EDGE_INDICES": "#d62728",
    "MIN_SUM_OF_MIN_EDGE_INDICES": "#9467bd",
}


def parse_filename(filename: str) -> dict | None:
    """Parse parameters from the filename."""
    pattern = r"n(\d+)_m(\d+)_(\w+?)_a([\d.]+)_(RANDOM|MAX_SUM_OF_MAX_EDGE_INDICES|MAX_SUM_OF_MIN_EDGE_INDICES|MIN_SUM_OF_MAX_EDGE_INDICES|MIN_SUM_OF_MIN_EDGE_INDICES)_rep(\d+)\.json"
    match = re.match(pattern, filename)
    if not match:
        return None
    return {
        "n": int(match.group(1)),
        "m": int(match.group(2)),
        "fitness_type": match.group(3),
        "fitness_alpha": match.group(4),
        "strategy": match.group(5),
        "rep": int(match.group(6)),
    }


def load_all_data(results_dir: str) -> list:
    """Load all result files and return list of (params, data) tuples."""
    all_data = []
    for filepath in sorted(glob.glob(os.path.join(results_dir, "*.json"))):
        filename = os.path.basename(filepath)
        params = parse_filename(filename)
        if params is None:
            continue
        with open(filepath, "r") as f:
            data = json.load(f)
        all_data.append((params, data))
    return all_data


def group_by_graph_params(all_data: list) -> dict:
    """Group data by (n, m, fitness_type, fitness_alpha)."""
    groups = defaultdict(lambda: defaultdict(list))
    for params, data in all_data:
        graph_key = (params["n"], params["m"], params["fitness_type"], params["fitness_alpha"])
        strategy = params["strategy"]
        groups[graph_key][strategy].append(data)
    return groups


def fitness_label(fitness_type: str, fitness_alpha: str) -> str:
    return f"{fitness_type}(α={fitness_alpha})"


# ============================================================
# Plot 1: Boxplot of num_edges by strategy
# ============================================================
def plot_num_edges_boxplot(groups: dict, output_dir: str):
    """For each (n, m, fitness) config, make a boxplot of num_edges across strategies."""
    print("\n--- Plot 1: Boxplot of final edge count by strategy ---")
    for graph_key, strategies in sorted(groups.items()):
        n, m, ft, fa = graph_key
        label = fitness_label(ft, fa)

        # Collect data per strategy
        strategy_names = []
        edge_counts = []
        colors = []
        for strat in STRATEGY_LABELS.keys():
            if strat in strategies:
                counts = [d["summary"]["num_edges"] for d in strategies[strat]]
                strategy_names.append(STRATEGY_LABELS[strat])
                edge_counts.append(counts)
                colors.append(STRATEGY_COLORS[strat])

        if not edge_counts:
            continue

        fig, ax = plt.subplots(figsize=(10, 5))
        bp = ax.boxplot(edge_counts, labels=strategy_names, patch_artist=True)
        for patch, color in zip(bp["boxes"], colors):
            patch.set_facecolor(color)
            patch.set_alpha(0.7)

        # Reference line: max planar edges = 3n - 6
        max_planar = 3 * n - 6
        ax.axhline(max_planar, color="gray", linestyle="--", alpha=0.7, label=f"3n-6={max_planar}")
        ax.legend()

        ax.set_ylabel("Liczba krawędzi w grafie końcowym", fontsize=11)
        ax.set_title(f"Końcowa liczba krawędzi vs strategia\nn={n}, m={m}, {label}", fontsize=12)
        ax.tick_params(axis="x", rotation=15)

        plt.tight_layout()
        out_path = os.path.join(output_dir, f"boxplot_edges_n{n}_m{m}_{ft}_a{fa}.png")
        plt.savefig(out_path, dpi=150, bbox_inches="tight")
        plt.close(fig)
        print(f"  Saved: {out_path}")


# ============================================================
# Plot 3: Edge count evolution over steps
# ============================================================
def plot_edge_evolution(groups: dict, output_dir: str):
    """For each config, plot cumulative edge count across steps, per strategy."""
    print("\n--- Plot 3: Edge count evolution over build steps ---")
    for graph_key, strategies in sorted(groups.items()):
        n, m, ft, fa = graph_key
        label = fitness_label(ft, fa)

        fig, ax = plt.subplots(figsize=(10, 6))

        for strat in STRATEGY_LABELS.keys():
            if strat not in strategies:
                continue
            data_list = strategies[strat]

            # For each repetition, compute cumulative edge count at each step
            all_curves = []
            for data in data_list:
                steps = data["steps"]
                cum_edges = []
                running = 0
                for step in steps:
                    running += step["edges_added"] - step["edges_removed"]
                    cum_edges.append(running)
                all_curves.append(cum_edges)

            # Align to same length (should be n-1 steps)
            max_len = max(len(c) for c in all_curves)
            aligned = np.full((len(all_curves), max_len), np.nan)
            for i, curve in enumerate(all_curves):
                aligned[i, :len(curve)] = curve

            mean_curve = np.nanmean(aligned, axis=0)
            std_curve = np.nanstd(aligned, axis=0)
            x = np.arange(1, max_len + 1)

            color = STRATEGY_COLORS[strat]
            ax.plot(x, mean_curve, label=STRATEGY_LABELS[strat], color=color, linewidth=1.5)
            ax.fill_between(x, mean_curve - std_curve, mean_curve + std_curve,
                            alpha=0.15, color=color)

        # Reference: max planar
        max_planar = 3 * n - 6
        ax.axhline(max_planar, color="gray", linestyle="--", alpha=0.7, label=f"3n-6={max_planar}")

        ax.set_xlabel("Krok (dodany wierzchołek)", fontsize=11)
        ax.set_ylabel("Skumulowana liczba krawędzi", fontsize=11)
        ax.set_title(f"Ewolucja liczby krawędzi w trakcie budowy grafu\nn={n}, m={m}, {label}", fontsize=12)
        ax.legend(fontsize=9, loc="lower right")
        ax.xaxis.set_major_locator(MaxNLocator(integer=True))

        plt.tight_layout()
        out_path = os.path.join(output_dir, f"evolution_n{n}_m{m}_{ft}_a{fa}.png")
        plt.savefig(out_path, dpi=150, bbox_inches="tight")
        plt.close(fig)
        print(f"  Saved: {out_path}")


# ============================================================
# Plot 5: Degree distribution comparison
# ============================================================
def plot_degree_distribution(groups: dict, output_dir: str):
    """For each config, plot average degree distribution (log-log) per strategy."""
    print("\n--- Plot 5: Degree distribution by strategy ---")
    for graph_key, strategies in sorted(groups.items()):
        n, m, ft, fa = graph_key
        label = fitness_label(ft, fa)

        fig, ax = plt.subplots(figsize=(9, 6))

        for strat in STRATEGY_LABELS.keys():
            if strat not in strategies:
                continue
            data_list = strategies[strat]

            # Aggregate degree sequences from all repetitions
            all_degrees = []
            for data in data_list:
                edges = data["graph"]["edges"]
                deg = defaultdict(int)
                for edge in edges:
                    deg[edge["source"]] += 1
                    deg[edge["target"]] += 1
                all_degrees.extend(deg.values())

            if not all_degrees:
                continue

            # Compute degree histogram
            max_deg = max(all_degrees)
            counts = np.zeros(max_deg + 1)
            for d in all_degrees:
                counts[d] += 1

            # Normalize to probability
            total = counts.sum()
            prob = counts / total

            # Plot only non-zero entries
            degrees = np.where(prob > 0)[0]
            probs = prob[degrees]

            color = STRATEGY_COLORS[strat]
            ax.scatter(degrees, probs, s=20, alpha=0.7, color=color, label=STRATEGY_LABELS[strat])

        ax.set_xscale("log")
        ax.set_yscale("log")
        ax.set_xlabel("Stopień wierzchołka k", fontsize=11)
        ax.set_ylabel("P(k)", fontsize=11)
        ax.set_title(f"Rozkład stopni wierzchołków\nn={n}, m={m}, {label}", fontsize=12)
        ax.legend(fontsize=9)
        ax.grid(True, alpha=0.3, which="both")

        plt.tight_layout()
        out_path = os.path.join(output_dir, f"degree_dist_n{n}_m{m}_{ft}_a{fa}.png")
        plt.savefig(out_path, dpi=150, bbox_inches="tight")
        plt.close(fig)
        print(f"  Saved: {out_path}")


# ============================================================
# Plot 4: Average edges removed per step
# ============================================================
def plot_edges_removed_per_step(groups: dict, output_dir: str):
    """For each config, plot the average number of edges removed at each step."""
    print("\n--- Plot 4: Average edges removed per step ---")
    for graph_key, strategies in sorted(groups.items()):
        n, m, ft, fa = graph_key
        label = fitness_label(ft, fa)

        fig, ax = plt.subplots(figsize=(10, 6))

        for strat in STRATEGY_LABELS.keys():
            if strat not in strategies:
                continue
            data_list = strategies[strat]

            # Collect edges_removed per step for each repetition
            all_curves = []
            for data in data_list:
                steps = data["steps"]
                removed = [step["edges_removed"] for step in steps]
                all_curves.append(removed)

            # Align to same length
            max_len = max(len(c) for c in all_curves)
            aligned = np.full((len(all_curves), max_len), np.nan)
            for i, curve in enumerate(all_curves):
                aligned[i, :len(curve)] = curve

            mean_curve = np.nanmean(aligned, axis=0)
            std_curve = np.nanstd(aligned, axis=0)
            x = np.arange(1, max_len + 1)

            color = STRATEGY_COLORS[strat]
            ax.plot(x, mean_curve, label=STRATEGY_LABELS[strat], color=color, linewidth=1.5)
            ax.fill_between(x, mean_curve - std_curve, mean_curve + std_curve,
                            alpha=0.15, color=color)

        ax.set_xlabel("Krok (dodany wierzchołek)", fontsize=11)
        ax.set_ylabel("Średnia liczba usuniętych krawędzi", fontsize=11)
        ax.set_title(f"Średnia liczba krawędzi usuwanych w każdym kroku\nn={n}, m={m}, {label}", fontsize=12)
        ax.legend(fontsize=9, loc="upper left")
        ax.xaxis.set_major_locator(MaxNLocator(integer=True))
        ax.set_ylim(bottom=0)

        plt.tight_layout()
        out_path = os.path.join(output_dir, f"removed_per_step_n{n}_m{m}_{ft}_a{fa}.png")
        plt.savefig(out_path, dpi=150, bbox_inches="tight")
        plt.close(fig)
        print(f"  Saved: {out_path}")


FITNESS_COLORS = {
    ("LINEAR", "1.0"): "#e41a1c",
    ("POLY", "0.2"): "#377eb8",
    ("POLY", "0.5"): "#4daf4a",
    ("POLY", "0.8"): "#984ea3",
    ("LOG", "1"): "#ff7f00",
}


def _get_fitness_color(ft: str, fa: str) -> str:
    return FITNESS_COLORS.get((ft, fa), "#333333")


# ============================================================
# Plot 6: Edge evolution grouped by (n, m), comparing fitness functions
# ============================================================
def plot_edge_evolution_by_fitness(all_data: list, output_dir: str):
    """For each (n, m), one figure with subplots per strategy, lines per fitness."""
    print("\n--- Plot 6: Edge evolution by fitness (grouped by n, m) ---")

    # Group by (n, m, strategy, fitness)
    grouped = defaultdict(lambda: defaultdict(lambda: defaultdict(list)))
    for params, data in all_data:
        n, m = params["n"], params["m"]
        strat = params["strategy"]
        fit_key = (params["fitness_type"], params["fitness_alpha"])
        grouped[(n, m)][strat][fit_key].append(data)

    for (n, m), strat_data in sorted(grouped.items()):
        strategies_present = [s for s in STRATEGY_LABELS.keys() if s in strat_data]
        num_strats = len(strategies_present)
        if num_strats == 0:
            continue

        fig, axes = plt.subplots(1, num_strats, figsize=(5 * num_strats, 5), sharey=True)
        if num_strats == 1:
            axes = [axes]

        for ax, strat in zip(axes, strategies_present):
            fitness_runs = strat_data[strat]
            for fit_key, data_list in sorted(fitness_runs.items()):
                ft, fa = fit_key
                # Compute mean edge evolution
                all_curves = []
                for data in data_list:
                    steps = data["steps"]
                    cum = 0
                    curve = []
                    for step in steps:
                        cum += step["edges_added"] - step["edges_removed"]
                        curve.append(cum)
                    all_curves.append(curve)

                max_len = max(len(c) for c in all_curves)
                aligned = np.full((len(all_curves), max_len), np.nan)
                for i, curve in enumerate(all_curves):
                    aligned[i, :len(curve)] = curve

                mean_curve = np.nanmean(aligned, axis=0)
                x = np.arange(1, max_len + 1)

                color = _get_fitness_color(ft, fa)
                ax.plot(x, mean_curve, label=fitness_label(ft, fa), color=color, linewidth=1.5)

            max_planar = 3 * n - 6
            ax.axhline(max_planar, color="gray", linestyle="--", alpha=0.5)
            ax.set_title(STRATEGY_LABELS[strat], fontsize=10)
            ax.set_xlabel("Krok", fontsize=9)
            ax.xaxis.set_major_locator(MaxNLocator(integer=True))

        axes[0].set_ylabel("Skumulowana liczba krawędzi", fontsize=10)
        axes[-1].legend(fontsize=8, loc="lower right")
        fig.suptitle(f"Ewolucja krawędzi — porównanie fitness\nn={n}, m={m}", fontsize=12)
        plt.tight_layout()
        out_path = os.path.join(output_dir, f"evolution_by_fitness_n{n}_m{m}.png")
        plt.savefig(out_path, dpi=150, bbox_inches="tight")
        plt.close(fig)
        print(f"  Saved: {out_path}")


# ============================================================
# Plot 7: Edges removed per step grouped by (n, m), comparing fitness
# ============================================================
def plot_removed_per_step_by_fitness(all_data: list, output_dir: str):
    """For each (n, m), one figure with subplots per strategy, lines per fitness."""
    print("\n--- Plot 7: Edges removed per step by fitness (grouped by n, m) ---")

    grouped = defaultdict(lambda: defaultdict(lambda: defaultdict(list)))
    for params, data in all_data:
        n, m = params["n"], params["m"]
        strat = params["strategy"]
        fit_key = (params["fitness_type"], params["fitness_alpha"])
        grouped[(n, m)][strat][fit_key].append(data)

    for (n, m), strat_data in sorted(grouped.items()):
        strategies_present = [s for s in STRATEGY_LABELS.keys() if s in strat_data]
        num_strats = len(strategies_present)
        if num_strats == 0:
            continue

        fig, axes = plt.subplots(1, num_strats, figsize=(5 * num_strats, 5), sharey=True)
        if num_strats == 1:
            axes = [axes]

        for ax, strat in zip(axes, strategies_present):
            fitness_runs = strat_data[strat]
            for fit_key, data_list in sorted(fitness_runs.items()):
                ft, fa = fit_key
                all_curves = []
                for data in data_list:
                    steps = data["steps"]
                    removed = [step["edges_removed"] for step in steps]
                    all_curves.append(removed)

                max_len = max(len(c) for c in all_curves)
                aligned = np.full((len(all_curves), max_len), np.nan)
                for i, curve in enumerate(all_curves):
                    aligned[i, :len(curve)] = curve

                mean_curve = np.nanmean(aligned, axis=0)
                x = np.arange(1, max_len + 1)

                color = _get_fitness_color(ft, fa)
                ax.plot(x, mean_curve, label=fitness_label(ft, fa), color=color, linewidth=1.5)

            ax.set_title(STRATEGY_LABELS[strat], fontsize=10)
            ax.set_xlabel("Krok", fontsize=9)
            ax.set_ylim(bottom=0)
            ax.xaxis.set_major_locator(MaxNLocator(integer=True))

        axes[0].set_ylabel("Śr. liczba usuniętych krawędzi", fontsize=10)
        axes[-1].legend(fontsize=8, loc="upper left")
        fig.suptitle(f"Krawędzie usuwane w kroku — porównanie fitness\nn={n}, m={m}", fontsize=12)
        plt.tight_layout()
        out_path = os.path.join(output_dir, f"removed_by_fitness_n{n}_m{m}.png")
        plt.savefig(out_path, dpi=150, bbox_inches="tight")
        plt.close(fig)
        print(f"  Saved: {out_path}")


def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    print("Loading data...")
    all_data = load_all_data(RESULTS_DIR)
    print(f"  Loaded {len(all_data)} result files.")

    groups = group_by_graph_params(all_data)
    print(f"  Found {len(groups)} graph configurations.")

    for graph_key, strategies in sorted(groups.items()):
        n, m, ft, fa = graph_key
        strat_counts = {s: len(v) for s, v in strategies.items()}
        print(f"  n={n}, m={m}, {ft}(α={fa}): {strat_counts}")

    plot_num_edges_boxplot(groups, OUTPUT_DIR)
    plot_edge_evolution(groups, OUTPUT_DIR)
    plot_edges_removed_per_step(groups, OUTPUT_DIR)
    plot_degree_distribution(groups, OUTPUT_DIR)
    plot_edge_evolution_by_fitness(all_data, OUTPUT_DIR)
    plot_removed_per_step_by_fitness(all_data, OUTPUT_DIR)

    print(f"\nDone! All plots saved to: {OUTPUT_DIR}")


if __name__ == "__main__":
    main()
