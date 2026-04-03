import matplotlib.pyplot as plt
import networkx as nx
from typing import List, Dict


class Drawer:
    """
    A utility class for drawing graphs and charts with a consistent style using Matplotlib and NetworkX.
    """
    
    COLORS = {
        "graph-node-color": "#5E81AC",
        "graph-node-alpha": 0.9,
        "graph-edge-color": "#2D3436",
        "graph-edge-alpha": 0.3,
        "graph-background-color": "#ECEFF4",
        "graph-background-alpha": 1.0,
        "color1": "#00D4FF",
        "color2": "#FF7675",
        "color3": "#55E6C1",
        "color4": "#A29BFE",
        "color5": "#2D3436"
    }
    

    def draw_graph(self, g: nx.Graph, title: str = "Graph Visualization", seed: int = 42) -> None:
        """
        Draws the given graph using Matplotlib and NetworkX with a consistent style.
        """
        plt.figure(figsize=(10, 10))
        plt.title(title)
        plt.axis('off')
        plt.gca().set_facecolor(self.COLORS["graph-background-color"])
        
        pos = nx.spring_layout(g, k=0.15, seed=seed)

        nx.draw_networkx_nodes(
            g, pos,
            node_size=200,
            node_color=self.COLORS["graph-node-color"],
            alpha=self.COLORS["graph-node-alpha"]
        )

        nx.draw_networkx_edges(
            g, pos,
            edge_color=self.COLORS["graph-edge-color"],
            alpha=self.COLORS["graph-edge-alpha"]
        )

        plt.show()

    def draw_line_chart(
        self, 
        x: List[float], 
        y_series: List[Dict[str, List[float]]], 
        title: str = "Line Chart", 
        xlabel: str = "X-axis", 
        ylabel: str = "Y-axis"
    ) -> None:
        plt.figure(figsize=(10, 6))
        
        for series in y_series:
            values = series.get("values", [])
            label = series.get("label", "Unknown")
            
            raw_color = series.get("color", "color1")
            actual_color = self.COLORS.get(raw_color, raw_color)
            
            plt.plot(
                x, 
                values, 
                label=label,
                linestyle=series.get("linestyle", "-"),
                marker=series.get("marker", "o"), 
                markersize=series.get("markersize", 5),
                color=actual_color,
                alpha=0.8,
                linewidth=2
            )

        plt.title(title, fontsize=14)
        plt.xlabel(xlabel)
        plt.ylabel(ylabel)
        plt.grid(True, linestyle='--', alpha=0.5)
        plt.legend()
        plt.tight_layout()
        plt.show()