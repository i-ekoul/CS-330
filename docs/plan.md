# Milestone 1 Plan — CS-330 (MainCode.cpp only, Spatial Picking)

**Branch:** `enhancement/m1-plan` | **Before tag:** `v-before-review`  
**Single file in scope:** `Source/MainCode.cpp`

## 1) Current State (file-level review)
- `MainCode.cpp` orchestrates init (GLFW/GLEW), constructs managers, enables depth testing, clears color/depth each frame, and calls `PrepareSceneView()` and `RenderScene()`.
- There is **no explicit picking path** in this file; a naïve approach would be **O(n)** (test a ray against every object).
- Input bindings and projection toggles are implemented elsewhere (e.g., `ViewManager`).

## 2) Gaps (why enhance)
- No documented entry point for picking from the main loop.
- No shared math helpers here for a robust **Ray–AABB (slab)** test or ray-construction notes.
- No broad phase to prune candidates before narrow-phase tests.

## 3) Enhancement Plan (single-file, plan only in M1)

### A. Minimal types (declared `static` in this file)
Add minimal data types and a robust slab-test helper with EPS guards.

```cpp
struct Ray  { glm::vec3 origin; glm::vec3 dir; }; // dir normalized
struct AABB { glm::vec3 min;   glm::vec3 max; };

// rayAabbSlab: returns true if ray hits AABB; sets tNear to nearest positive t.
// Pre: ray.dir is normalized; EPS guards divisions by near-zero components.
static bool rayAabbSlab(const Ray& r, const AABB& box, float& tNear) {
    const float EPS = 1e-6f;
    float tmin = 0.0f, tmax = std::numeric_limits<float>::infinity();
    for (int i = 0; i < 3; ++i) {
        float o = (&r.origin.x)[i], d = (&r.dir.x)[i];
        float bmin = (&box.min.x)[i], bmax = (&box.max.x)[i];
        if (std::abs(d) < EPS) {
            if (o < bmin || o > bmax) return false; // parallel and outside
        } else {
            float inv = 1.0f / d;
            float t1 = (bmin - o) * inv, t2 = (bmax - o) * inv;
            if (t1 > t2) std::swap(t1, t2);
            tmin = std::max(tmin, t1);
            tmax = std::min(tmax, t2);
            if (tmin > tmax) return false;
        }
    }
    tNear = (tmin >= 0.0f) ? tmin : 0.0f; // inside box counts as hit at t=0
    return true;
}
````

### B. Broad-phase choice (uniform grid)

Document a simple **uniform grid** approach (parameters only in M1):

* Define `cellSize` and scene `bounds`.
* Use **3D-DDA traversal** along the ray to gather **candidate IDs**.
* Perform narrow phase with `rayAabbSlab` on gathered candidates.
  *(Implementation deferred to M2.)*

### C. Picking entry point

Declare a placeholder and mark the call site with a comment in the loop.

```cpp
static int pickObjectId(const Ray& r); // nearest hit ID or -1
// In main loop (comment only for M1):
// if (mouseClicked) {
//     Ray rw = screenToRay(mouseX, mouseY); // via current camera in ViewManager
//     int id = pickObjectId(rw);
// }
```

### D. In-file documentation

Add a short **“Picking Plan”** comment at the top of `MainCode.cpp`:

* Baseline **O(n)** → **uniform grid** broad phase + **slab** narrow phase.
* Numeric guard **EPS = 1e-6**.
* Note that input/projection toggles are handled outside this file.

## 4) Outcomes Mapping (CS-499)

* **CO3 (Design):** single-file integration point; responsibilities documented at the loop.
* **CO2 (Algorithms):** uniform-grid broad phase + slab narrow phase with numeric guards.
* **CO4 (Assessment):** in M2, show two deterministic rays (hit/miss) and candidate counts vs. O(n).
* **CO5 (Docs/Policies):** concise in-file comments describing assumptions, EPS, and call site.

## 5) Screencast Talking Points (M1)

1. Open `v-before-review` → show `Source/MainCode.cpp` loop and absence of picking.
2. State **CO3/CO2/CO4/CO5** mapping.
3. Note that implementation/evidence lands does not occur in M1.
