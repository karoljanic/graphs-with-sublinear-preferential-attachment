import os
import json
import itertools
import networkx as nx
from src.planar_preferential_attachment_graph import PlanarPreferentialAttachmentGraph, FitnessType, EdgeRemovalStrategy


def get_result_filepath(params: dict, results_dir: str) -> str:
    """Get the filepath for a given parameter combination."""
    filename = (
        f"n{params['n']}_m{params['m']}_{params['fitness_type'].name}_a{params['fitness_alpha']}"
        f"_{params['strategy'].name}_rep{params['rep']}.json"
    )
    return os.path.join(results_dir, filename)


def is_already_done(params: dict, results_dir: str) -> bool:
    """Check if a result file already exists and is valid."""
    filepath = get_result_filepath(params, results_dir)
    if not os.path.exists(filepath):
        return False
    try:
        with open(filepath, "r") as f:
            data = json.load(f)
        return "summary" in data and "steps" in data and "graph" in data
    except (json.JSONDecodeError, IOError):
        return False


def generate_single(params: dict, results_dir: str) -> dict:
    """Generate a single planar PA graph and return results dict."""
    n = params["n"]
    m = params["m"]
    fitness_type = params["fitness_type"]
    fitness_alpha = params["fitness_alpha"]
    strategy = params["strategy"]
    rep = params["rep"]

    G = PlanarPreferentialAttachmentGraph(
        n=n,
        m=m,
        fitness=fitness_type,
        fitness_alpha=fitness_alpha,
        strategy=strategy,
        track_history=True,
    )

    steps = [
        {
            "step": s.step,
            "new_node_id": s.new_node_id,
            "edges_added": s.edges_added,
            "edges_removed": s.edges_removed,
            "was_planar_before_removal": s.was_planar_before_removal,
        }
        for s in G.step_info
    ]

    graph_data = {
        "nodes": list(G.nodes()),
        "edges": [{"source": u, "target": v} for u, v in G.edges()],
    }

    total_added = sum(s["edges_added"] for s in steps)
    total_removed = sum(s["edges_removed"] for s in steps)
    non_planar_steps = sum(1 for s in steps if not s["was_planar_before_removal"])

    result = {
        "params": {
            "n": n,
            "m": m,
            "fitness_type": fitness_type.name,
            "fitness_alpha": fitness_alpha,
            "strategy": strategy.name,
            "rep": rep,
        },
        "summary": {
            "num_nodes": G.number_of_nodes(),
            "num_edges": G.number_of_edges(),
            "total_edges_added": total_added,
            "total_edges_removed": total_removed,
            "non_planar_steps": non_planar_steps,
            "is_planar": nx.is_planar(G),
        },
        "steps": steps,
        "graph": graph_data,
    }

    filepath = get_result_filepath(params, results_dir)
    os.makedirs(os.path.dirname(filepath) or '.', exist_ok=True)
    with open(filepath, "w") as f:
        json.dump(result, f, indent=2)

    return result
