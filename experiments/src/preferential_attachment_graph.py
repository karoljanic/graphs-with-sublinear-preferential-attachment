import networkx as nx
import subprocess
import tempfile
import os
import json
from enum import Enum
from typing import Dict, List
import glob


class FitnessType(Enum):
    LINEAR = 0
    POLY = 1
    LOG = 2


class PreferentialAttachmentGraph(nx.Graph):
    """
    Preferential Attachment Graph With Custom Fitness Function
    """

    def __init__(
        self,
        n: int = 0,
        m: int = 1,
        fitness: FitnessType = FitnessType.LINEAR,
        fitness_alpha: float = 1.0,
        track_history: bool = False,
        select_planar_subgraph: bool = False,
        test_planarity_restoring: bool = False
    ):
        """
        Args:
            n: Total number of nodes in the final graph.
            m: Number of edges to attach from a new node to existing nodes.
            fitness: One of the predefined fitness enums (LINEAR, POLY, LOG).
            fitness_alpha: Parameter alpha used for POLY and LOG fitness types.
            track_history: Whether to track the history of graph modifications.
            select_planar_subgraph: Whether to select a planar subgraph.
            test_planarity_restoring: Whether to run the planarity restoring test after generation.
        """
        super().__init__()
            
        self._n = n
        self._m = m
        self._fitness = fitness
        self._fitness_alpha = fitness_alpha
        self._track_history = track_history
        self._select_planar_subgraph = select_planar_subgraph
        self._test_planarity_restoring = test_planarity_restoring
        self._cumulative_weights = []
        self._cumulative_degrees = []
        self._restoring_test_result = []

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
    def cumulative_weights(self) -> List[float]:
        """Cumulative weight sums recorded by the C++ generator (if available)."""
        return self._cumulative_weights

    @property
    def cumulative_degrees(self) -> List[List[float]]:
        """Per-step degree snapshots recorded by the C++ generator."""
        return self._cumulative_degrees
    
    @property
    def restoring_test_result(self) -> List[dict]:
        """Returns a list of parsed JSON objects produced by `test_planarity_restoring`."""
        return self._restoring_test_result

    @property
    def track_history(self) -> bool:
        return self._track_history

    def __repr__(self) -> str:
        fitness_name = self.fitness.name if hasattr(self.fitness, 'name') else str(self.fitness)
        return f"PreferentialAttachmentGraph(n={self.n}, m={self.m}, fitness={fitness_name}, alpha={self._fitness_alpha})"

    @staticmethod
    def _read_weights_file(filename: str) -> List[float]:
        """Read a weights file (one float per line). Returns list of floats or empty list on error."""
        try:
            if not os.path.isfile(filename):
                return []
            with open(filename, "r") as wf:
                return [float(line.strip()) for line in wf if line.strip()]
        except Exception:
            return []

    @staticmethod
    def _read_degrees_file(filename: str) -> List[List[float]]:
        """Read a degrees tracking file."""
        node_degrees: Dict[int, List[float]] = {}
        try:
            if not os.path.isfile(filename):
                return {}
            
            with open(filename, "r") as df:
                for t, line in enumerate(df):
                    line = line.strip()
                    if not line:
                        continue
                    
                    snapshot = [float(x) for x in line.split()][:t+2]
                    
                    for node_id, degree in enumerate(snapshot):
                        if node_id not in node_degrees:
                            node_degrees[node_id] = [0.0] * t
                        
                        node_degrees[node_id].append(degree)
                        
        except Exception:
            return {}
            
        return node_degrees

    def _read_tracking_files(self, prefix: str) -> tuple:
        """Given the temp-file prefix, read both weights and degrees tracking files.
        Returns the actual file paths (weights_file, degrees_file)."""
        weights_file = prefix + "_weights.txt"
        degrees_file = prefix + "_degrees.txt"
        self._cumulative_weights = PreferentialAttachmentGraph._read_weights_file(weights_file)
        self._cumulative_degrees = PreferentialAttachmentGraph._read_degrees_file(degrees_file)
        return weights_file, degrees_file
    
    def _generate_graph(self) -> None:
        """
        Generates the graph by calling an external C++ binary generator.
        """

        gen_bin = self._find_bin("generate_pa")
        gen_track_bin = self._find_bin("generate_and_track_pa")

        if self._track_history:
            cmd = gen_track_bin if os.path.isfile(gen_track_bin) else None
        else:
            cmd = gen_bin if os.path.isfile(gen_bin) else None

        if cmd is None:
            raise RuntimeError("C++ generator binary not found. Please ensure it is built and accessible.")

        tf = tempfile.NamedTemporaryFile(prefix="pa_gen_", suffix=".json", delete=False)
        tf_path = tf.name
        tf.close()

        fitness_type_arg = str(self._fitness.value)
        fitness_alpha_arg = str(self._fitness_alpha)

        proc_args = [cmd, tf_path, str(self.n), str(self.m), fitness_type_arg, fitness_alpha_arg]

        try:
            subprocess.run(proc_args, check=True)

            planar_tf_path = None
            if self._select_planar_subgraph:
                select_bin = PreferentialAttachmentGraph._find_bin("select_planar_subgraph")
                if not os.path.isfile(select_bin):
                    raise RuntimeError(f"select_planar_subgraph binary not found: {select_bin}")

                planar_tf = tempfile.NamedTemporaryFile(prefix="pa_planar_", suffix=".json", delete=False)
                planar_tf_path = planar_tf.name
                planar_tf.close()

                try:
                    subprocess.run([select_bin, tf_path, planar_tf_path], check=True)
                except Exception:
                    try:
                        os.remove(planar_tf_path)
                    except Exception:
                        pass
                    raise RuntimeError("Failed to run select_planar_subgraph binary")
        except Exception:
            try:
                os.remove(tf_path)
            except Exception:
                pass
            raise RuntimeError("Failed to run the C++ generator binary. Please ensure it is built and accessible.")

        try:
            json_path = planar_tf_path if (self._select_planar_subgraph and planar_tf_path is not None) else tf_path
            with open(json_path, "r") as fh:
                data = json.load(fh)

            self.clear()

            nodes = data.get("nodes", [])
            if nodes:
                for node in nodes:
                    nid = node.get("id")
                    if nid is None:
                        continue
                    self.add_node(nid)
            else:
                self.add_nodes_from(range(self.n))

            for edge in data.get("edges", []):
                s = edge.get("source")
                t = edge.get("target")
                if s is None or t is None:
                    continue
                self.add_edge(s, t)

            if self._track_history:
                weights_file, degrees_file = self._read_tracking_files(tf_path)

            # If requested, run the planarity restoring test and collect results
            if self._test_planarity_restoring:
                test_bin = PreferentialAttachmentGraph._find_bin("test_planarity_restoring")
                if not os.path.isfile(test_bin):
                    raise RuntimeError(f"test_planarity_restoring binary not found: {test_bin}")

                # create prefix for outputs (directory + base name)
                out_dir = tempfile.mkdtemp(prefix="pa_restoring_")
                out_prefix = os.path.join(out_dir, "restoring_")

                try:
                    subprocess.run([test_bin, json_path, out_prefix], check=True)
                except Exception:
                    # attempt to remove created files/dir, but don't mask the exception
                    try:
                        for f in glob.glob(out_prefix + "*.json"):
                            os.remove(f)
                    except Exception:
                        pass
                    try:
                        os.rmdir(out_dir)
                    except Exception:
                        pass
                    raise RuntimeError("Failed to run test_planarity_restoring binary")

                # gather produced JSON files
                results = []
                for fpath in sorted(glob.glob(out_prefix + "*.json")):
                    try:
                        with open(fpath, "r") as fh:
                            results.append(json.load(fh))
                    except Exception:
                        # skip malformed files
                        continue

                self._restoring_test_result = results
                print(f"Planarity restoring test produced {len(results)} result files.")
                try:
                    for fpath in glob.glob(out_prefix + "*.json"):
                        os.remove(fpath)
                except Exception:
                    pass
                try:
                    os.rmdir(out_dir)
                except Exception:
                    pass
        finally:
            try:
                if os.path.isfile(tf_path):
                    os.remove(tf_path)
            except Exception:
                pass

            try:
                if planar_tf_path is not None and os.path.isfile(planar_tf_path):
                    os.remove(planar_tf_path)
            except Exception:
                pass

            try:
                try:
                    os.remove(weights_file)
                except Exception:
                    pass
                try:
                    os.remove(degrees_file)
                except Exception:
                    pass
            except Exception:
                pass

    @staticmethod
    def _find_bin(binary_name: str, bin_dir: str = None) -> str:
        """Return absolute path to binary; look in provided bin_dir or default code/build/bin."""
        repo_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
        search_dir = bin_dir if bin_dir is not None else os.path.join(repo_dir, "code", "build", "bin")
        bin_path = os.path.join(search_dir, binary_name)
        return bin_path

    @staticmethod
    def run_test_weight_sums(repeat_count: int, num_nodes: int, edges_per_node: int,
                             fitness: FitnessType = FitnessType.LINEAR, fitness_alpha: float = 1.0) -> dict:
        """Run the C++ test `test_pa_weight_sums` and return run info.

        Returns dict with keys: returncode, stdout, stderr, output_file
        """
        bin_path = PreferentialAttachmentGraph._find_bin("test_pa_weight_sums")
        if not os.path.isfile(bin_path):
            raise RuntimeError(f"Binary not found: {bin_path}")

        tf = tempfile.NamedTemporaryFile(prefix="pa_test_weights_", suffix=".out", delete=False)
        output_file = tf.name
        tf.close()

        args = [bin_path, output_file, str(repeat_count), str(num_nodes), str(edges_per_node), str(fitness.value), str(fitness_alpha)]
        proc = subprocess.run(args, capture_output=True, text=True)

        averaged_weights = None
        try:
            averaged_weights = PreferentialAttachmentGraph._read_weights_file(output_file) if os.path.isfile(output_file) else None
        except Exception:
            averaged_weights = None

        return {
            "returncode": proc.returncode,
            "stdout": proc.stdout,
            "stderr": proc.stderr,
            "output_file": output_file,
            "averaged_weights": averaged_weights,
        }

    @staticmethod
    def run_test_degrees(repeat_count: int, num_nodes: int, edges_per_node: int,
                         fitness: FitnessType = FitnessType.LINEAR, fitness_alpha: float = 1.0) -> dict:
        """Run the C++ test `test_pa_degrees` and return run info.

        Returns dict with keys: returncode, stdout, stderr, output_file
        """
        bin_path = PreferentialAttachmentGraph._find_bin("test_pa_degrees")
        if not os.path.isfile(bin_path):
            raise RuntimeError(f"Binary not found: {bin_path}")

        tf = tempfile.NamedTemporaryFile(prefix="pa_test_degrees_", suffix=".out", delete=False)
        output_file = tf.name
        tf.close()

        args = [bin_path, output_file, str(repeat_count), str(num_nodes), str(edges_per_node), str(fitness.value), str(fitness_alpha)]
        proc = subprocess.run(args, capture_output=True, text=True)
        averaged_degrees = None
        try:
            averaged_degrees = PreferentialAttachmentGraph._read_degrees_file(output_file) if os.path.isfile(output_file) else None
        except Exception:
            averaged_degrees = None

        return {
            "returncode": proc.returncode,
            "stdout": proc.stdout,
            "stderr": proc.stderr,
            "output_file": output_file,
            "averaged_degrees": averaged_degrees,
        }
