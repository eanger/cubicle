To do:
* workers oscillate when they can't reach a target that's too close to a collideable
* zooming always grows out and in from the origin. 
* Rather than having a zoom ratio, we should have a zoom level where the ratio is calculated, so we don't run into rounding issues if we zoom a ton
* switch actions set to a queue, so we handle when multiple actions of the same type happen in between frames
* Add wall entity (paintable) that uses the correct side of the wall depending on edges and stuff.
* Prevent chairs from being placed on top of each other.
    Maybe zones can be painted on for irregular shapes, using flood fill or something similar.
    We might need to search grid space using something like a quadtree.
* Units have a bounding box for collision? To prevent placing objects on top of other objects (ie can only place if no collision)
* merge collideables and occupied locations so that they're kept in sync?
* Change blend for brush only when it's hovering over a location that's occupied
* Make it so you can't build a chair plan over top of another chair plan


Pathfinding
-----------
Invariants:
    - The open list is ordered by the cumulative cost to reach that node
Procedure:
    - Add starting node to open list
    - Pop an element E off the open list
        - For each reachable neighbor N of E
            - If N is the destination, we're done
            - If N is in the open list, update it's movement cost as necessary
            - Set the parent of N to E
            - Add N to the open list
            - Put E on the closed list
