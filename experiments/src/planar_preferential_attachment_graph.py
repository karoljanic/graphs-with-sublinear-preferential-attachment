import networkx as nx
import os
import json
import subprocess
import tempfile
from enum import Enum
from typing import List, Optional
from dataclasses import dataclass


class FitnessType(Enum):
    LINEAR = 0
    POLY = 1
    LOG = 2


class EdgeRemovalStrategy(Enum):
    MIN_SUM_OF_MIN_EDGE_INDICES = 0
    MAX_SUM_OF_MIN_EDGE_INDICES = 1
    MIN_SUM_OF_MAX_EDGE_INDICES = 2
    MAX_SUM_OF_MAX_EDGE_INDICES = 3
    RANDOM = 4


@dataclass
class StepInfo:
    step: int
    new_node_id: int
    edges_added: int
    edges_removed: int
    was_planar_before_removal: bool


class PlanarPreferentialAttachmentGraph(nx.Graph):
    """
    Planar Preferential Attachment Graph With Custom Fitness Function
    """

    def __init__(
        self,
        n: int = 0,
        m: int = 1,
        fitness: FitnessType = FitnessType.LINEAR,
        fitness_alpha: float = 1.0,
        strategy: EdgeRemovalStrategy = EdgeRemovalStrategy.RANDOM,
        track_history: bool = False
    ):
        """
        Args:
            n: Total number of nodes in the final graph
            m: Number of edges to attach from a new node to existing nodes
            fitness: One of the predefined fitness enums (LINEAR, POLY, LOG)
            fitness_alpha: Parameter alpha used for POLY and LOG fitness types
            strategy: Strategy for selecting which edges to remove when restoring planarity
            track_history: Whether to track the history (currently unused)
        """
        super().__init__()
        
        self._n = n
        self._m = m
        self._fitness = fitness
        self._fitness_alpha = fitness_alpha
        self._strategy = strategy
        self._track_history = track_history
        self._step_info: List[StepInfo] = []
        
        self._generate_graph()

    @property
    def n(self) -> int:
        return self._n
    
    @property
    def m(self) -> int:
        return self._m
    
    @property
    def fitness(self) -> FitnessType:
        return self._fitness

    @property
    def fitness_alpha(self) -> float:
        return self._fitness_alpha
    
    @property
    def strategy(self) -> EdgeRemovalStrategy:
        return self._strategy

    @property
    def step_info(self) -> List[StepInfo]:
        """Get list of step information records"""
        return self._step_info
    
    @property
    def track_history(self) -> bool:
        return self._track_history
    
    def __repr__(self) -> str:
        fitness_name = self.fitness.name if hasattr(self.fitness, 'name') else str(self.fitness)
        strategy_name = self.strategy.name if hasattr(self.strategy, 'name') else str(self.strategy)
        return (f"PlanarPreferentialAttachmentGraph(n={self.n}, m={self.m}, "
                f"fitness={fitness_name}, fitness_alpha={self.fitness_alpha}, "
                f"strategy={strategy_name})")

    def _generate_graph(self) -> None:
        """
        Generates the graph by calling the C++ binary generator.
        """

        gen_bin = self._find_bin("generate_planar_pa")

        tf = tempfile.NamedTemporaryFile(prefix="planar_pa_gen_", suffix=".json", delete=False)
        tf_path = tf.name
        tf.close()

        fitness_type_arg = str(self._fitness.value)
        fitness_alpha_arg = str(self._fitness_alpha)
        strategy_arg = str(self._strategy.value)

        proc_args = [
            gen_bin,
            tf_path,
            str(self._n),
            str(self._m),
            fitness_type_arg,
            strategy_arg,
            fitness_alpha_arg,
        ]

        try:
            subprocess.run(proc_args, check=True, capture_output=True)
        except subprocess.CalledProcessError as e:
            try:
                os.remove(tf_path)
            except Exception:
                pass
            raise RuntimeError("Failed to run the C++ generator binary. Please ensure it is built and accessible.")

        try:
            with open(tf_path, "r") as fh:
                data = json.load(fh)

            self.clear()

            nodes = data.get("nodes", [])
            if nodes:
                for node in nodes:
                    nid = node.get("id")
                    if nid is not None:
                        self.add_node(nid)
            else:
                self.add_nodes_from(range(self._n))

            for edge in data.get("edges", []):
                s = edge.get("source")
                t = edge.get("target")
                if s is not None and t is not None:
                    self.add_edge(s, t)

            # Always parse step info from C++ output
            self._parse_step_info(data.get("steps", []))

        finally:
            try:
                os.remove(tf_path)
            except Exception:
                pass

    def _parse_step_info(self, steps_data: List[dict]) -> None:
        """Parse step information from C++ output"""
        self._step_info = []
        for step_data in steps_data:
            step_info = StepInfo(
                step=step_data.get("step", 0),
                new_node_id=step_data.get("new_node_id", 0),
                edges_added=step_data.get("edges_added", 0),
                edges_removed=step_data.get("edges_removed", 0),
                was_planar_before_removal=step_data.get("was_planar_before_removal", True),
            )
            self._step_info.append(step_info)

    @staticmethod
    def _find_bin(binary_name: str, bin_dir: str = None) -> str:
        """Return absolute path to binary; look in provided bin_dir or default code/build/bin."""
        repo_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
        search_dir = bin_dir if bin_dir is not None else os.path.join(repo_dir, "code", "build", "bin")
        bin_path = os.path.join(search_dir, binary_name)
        return bin_path


if __name__ == "__main__":
    print("=" * 60)
    print("Planar Preferential Attachment Graph Test")
    print("=" * 60)

    try:
        # Test with LINEAR fitness, MIN_SUM_OF_MIN strategy
        graph = PlanarPreferentialAttachmentGraph(
            n=100,
            m=2,
            fitness=FitnessType.LINEAR,
            fitness_alpha=1.0,
            strategy=EdgeRemovalStrategy.RANDOM,
            track_history=True
        )

        print(f"\n✓ Graph generated successfully!")
        print(f"  Nodes: {graph.number_of_nodes()}")
        print(f"  Edges: {graph.number_of_edges()}")
        print(f"  Is planar: {nx.is_planar(graph)}")

        print(f"\nStep Statistics:")
        print(f"  Total steps: {len(graph.step_info)}")

        if graph.step_info:
            total_added = sum(s.edges_added for s in graph.step_info)
            total_removed = sum(s.edges_removed for s in graph.step_info)
            non_planar_steps = sum(
                1 for s in graph.step_info if not s.was_planar_before_removal
            )

            print(f"  Total edges added: {total_added}")
            print(f"  Total edges removed: {total_removed}")
            print(f"  Non-planar steps: {non_planar_steps}")

            # Print step history
            print(f"\nStep History (first 10 and last 5):")
            print(f"  {'Step':<6} {'Node':<6} {'Added':<8} {'Removed':<10} {'Planar':<8}")
            print(f"  " + "-" * 44)
            
            # First 10 steps
            for info in graph.step_info[:10]:
                planar_str = "YES" if info.was_planar_before_removal else "NO"
                print(f"  {info.step:<6} {info.new_node_id:<6} {info.edges_added:<8} {info.edges_removed:<10} {planar_str:<8}")
            
            if len(graph.step_info) > 15:
                print(f"  ... ({len(graph.step_info) - 15} more steps)")
            
            # Last 5 steps
            for info in graph.step_info[-5:]:
                planar_str = "YES" if info.was_planar_before_removal else "NO"
                print(f"  {info.step:<6} {info.new_node_id:<6} {info.edges_added:<8} {info.edges_removed:<10} {planar_str:<8}")

        print("\n" + "=" * 60)

    except Exception as e:
        print(f"\n✗ Error: {e}")
        print("\nMake sure 'generate_planar_pa' is built:")
        print("  cd code/build && cmake .. && make generate_planar_pa -j4")
