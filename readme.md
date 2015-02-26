To do:
* workers oscillate when they can't reach a target that's too close to a collideable
* zooming always grows out and in from the origin. this should grow into a fully-fleshed mapping from world space to camera space
* Rather than having a zoom ratio, we should have a zoom level where the ratio is calculated, so we don't run into rounding issues if we zoom a ton
* switch actions set to a queue, so we handle when multiple actions of the same type happen in between frames


Partition world into grid so that objects can't be placed over each other. This will help with pathfinding and zones (ie breakrooms, workspaces, offices, etc).
    Maybe zones can be painted on for irregular shapes, using flood fill or something similar.
    We might need to search grid space using something like a quadtree.
