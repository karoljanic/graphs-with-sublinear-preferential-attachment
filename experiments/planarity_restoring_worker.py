import json
import os
import networkx as nx
from src.preferential_attachment_graph import PreferentialAttachmentGraph, FitnessType


def get_result_filepath(params: dict, results_dir: str) -> str:
    filename = (
        f"n{params['n']}_m{params['m']}_{params['fitness_type'].name}_a{params['fitness_alpha']}_rep{params['rep']}_restoring.json"
    )
    return os.path.join(results_dir, filename)


def is_already_done(params: dict, results_dir: str) -> bool:
    filepath = get_result_filepath(params, results_dir)
    if not os.path.exists(filepath):
        return False
    try:
        with open(filepath, "r") as f:
            data = json.load(f)
        return "restoring_results" in data and "summary" in data and "params" in data
    except Exception:
        return False


def generate_single(params: dict, results_dir: str) -> dict:
    n = params["n"]
    m = params["m"]
    fitness_type = params["fitness_type"]
    fitness_alpha = params["fitness_alpha"]
    rep = params["rep"]

    G = PreferentialAttachmentGraph(
        n=n,
        m=m,
        fitness=fitness_type,
        fitness_alpha=fitness_alpha,
        track_history=False,
        select_planar_subgraph=True,
        test_planarity_restoring=True,
    )

    restoring_results = G.restoring_test_result

    graph_data = {
        "nodes": list(G.nodes()),
        "edges": [{"source": u, "target": v} for u, v in G.edges()],
    }

    result = {
        "params": {
            "n": n,
            "m": m,
            "fitness_type": fitness_type.name,
            "fitness_alpha": fitness_alpha,
            "rep": rep,
        },
        "summary": {
            "num_nodes": G.number_of_nodes(),
            "num_edges": G.number_of_edges(),
            "num_restoring_cases": len(restoring_results),
            "is_planar": nx.is_planar(G),
        },
        "restoring_results": restoring_results,
        "graph": graph_data,
    }

    filepath = get_result_filepath(params, results_dir)
    with open(filepath, "w") as f:
        json.dump(result, f, indent=2)

    return result
