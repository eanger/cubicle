To do:
* workers oscillate when they can't reach a target that's too close to a collideable
* zooming always grows out and in from the origin. this should grow into a fully-fleshed mapping from world space to camera space
* Rather than having a zoom ratio, we should have a zoom level where the ratio is calculated, so we don't run into rounding issues if we zoom a ton
* switch actions set to a queue, so we handle when multiple actions of the same type happen in between frames
