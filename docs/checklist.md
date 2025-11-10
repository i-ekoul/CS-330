# Code Review Checklist (CS-330, Source/MainCode.cpp)

> Scope: **one file** — `Source/MainCode.cpp`  
> Baseline: `v-before-review` tag

---

## A) Loop & Integration
- [ ] Main render loop remains the single integration point (no spatial logic scattered elsewhere).
- [ ] Comment indicates where screen→ray conversion would call `pickObjectId(...)`.

**Before (notes):** No picking entry; loop only orchestrates view/scene.

---

## B) Minimal Math Helpers (declared here)
- [ ] `struct Ray { origin, dir(normalized) }` and `struct AABB { min, max }` present (or documented as plan in M1).
- [ ] `rayAabbSlab(ray, aabb, tNear)` documented with EPS guards and pre/postconditions.

**Before (notes):** No shared picking math in this file.

---

## C) Broad-Phase Choice 
- [ ] Uniform-grid strategy recorded (cellSize, scene bounds, 3D-DDA traversal).
- [ ] Candidate-then-narrow-phase flow described (plan only for M1).

**Before (notes):** No broad-phase plan documented.

---

## D) Numerical Edge Cases (documented)
- [ ] Handle `|dir| ≈ 0` per axis with EPS (parallel slab case).
- [ ] Ray origin inside AABB clarified (accept `t = 0` as hit start).

---

## E) Evidence Plan 
- [ ] Define 2 deterministic rays (1 hit, 1 miss) for a fixed camera pose.
- [ ] Record candidate counts vs. O(n) baseline; note whether slab test selects the same nearest hit.

---

## Rubric Mapping (CS-499)
- **CO3** clear single-file integration boundary  
- **CO2** algorithmic reasoning (uniform grid + slab, EPS)  
- **CO4** simple, deterministic verification plan  
- **CO5** concise, in-file documentation of assumptions and call site
