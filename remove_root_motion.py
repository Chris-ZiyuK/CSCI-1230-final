#!/usr/bin/env python3
"""
Script to remove root motion (translation) from GLB animations
while keeping internal movements (rotation/scale) like wing flapping, tail wagging, etc.
"""

import sys
from pygltflib import GLTF2

def remove_root_motion(input_file: str, output_file: str, root_node_name: str = "root ground_01"):
    """
    Remove translation channels for the root node from all animations.
    
    Args:
        input_file: Path to input GLB file
        output_file: Path to output GLB file
        root_node_name: Name of the root node to filter (default: "root ground_01")
    """
    print(f"Loading GLB file: {input_file}")
    try:
        gltf = GLTF2().load(input_file)
    except Exception as e:
        print(f"Error loading GLB file: {e}")
        sys.exit(1)
    
    # Find the root node by name
    target_node_index = -1
    for i, node in enumerate(gltf.nodes):
        if node.name == root_node_name:
            target_node_index = i
            break
    
    if target_node_index == -1:
        print(f"Error: Could not find node '{root_node_name}'")
        print("Available node names:")
        for i, node in enumerate(gltf.nodes):
            if node.name:
                print(f"  [{i}] {node.name}")
        sys.exit(1)
    else:
        print(f"Found target node index: {target_node_index}")
    
    # Process animations
    if not gltf.animations:
        print("No animations found in the file.")
        return
    
    print(f"\nProcessing {len(gltf.animations)} animation(s)...")
    
    # Traverse all animations, remove translation channels targeting the root node
    removed_count = 0
    for anim_idx, anim in enumerate(gltf.animations):
        anim_name = anim.name if anim.name else f"Animation_{anim_idx}"
        original_len = len(anim.channels)
        
        # Use list comprehension to keep channels that should NOT be removed
        # Keep: (target is not the root node) OR (target is root node but path is not translation)
        anim.channels = [
            ch for ch in anim.channels 
            if not (ch.target.node == target_node_index and ch.target.path == "translation")
        ]
        
        removed = original_len - len(anim.channels)
        removed_count += removed
        if removed > 0:
            print(f"  Animation {anim_idx} '{anim_name}': removed {removed} translation channel(s)")
    
    print(f"\nTotal removed: {removed_count} translation channels")
    
    # Save the modified GLB
    print(f"\nSaving modified GLB to: {output_file}")
    try:
        gltf.save(output_file)
        print("Success! Root motion removed.")
    except Exception as e:
        print(f"Error saving GLB file: {e}")
        sys.exit(1)

if __name__ == "__main__":
    input_file = "resources/models/desert-titan/Desert Titan.glb"
    output_file = "resources/models/desert-titan/Desert Titan_InPlace.glb"
    root_node_name = "root ground_01"
    
    # Allow command line arguments
    if len(sys.argv) > 1:
        input_file = sys.argv[1]
    if len(sys.argv) > 2:
        output_file = sys.argv[2]
    if len(sys.argv) > 3:
        root_node_name = sys.argv[3]
    
    remove_root_motion(input_file, output_file, root_node_name)
