# CS-499 Milestone 1 — Code Review (CS-330)

**Artifact:** `CS330_3D_Camping_Scene/Source/MainCode.cpp`  
**Scope:** Single file only (no other files changed)
**Baseline tag:** `v-before-review`  
**Plan branch:** `enhancement/m1-plan`

---

## Quick links
- Before tag: https://github.com/i-ekoul/CS-330/releases/tag/v-before-review
- Enhancement branch: https://github.com/i-ekoul/CS-330/tree/enhancement/m1-plan
- Plan (docs/plan.md): https://github.com/i-ekoul/CS-330/blob/enhancement/m1-plan/docs/plan.md
- Checklist (docs/checklist.md): https://github.com/i-ekoul/CS-330/blob/enhancement/m1-plan/docs/checklist.md

---

## What this video will show
1. Open the **before tag** and show `MainCode.cpp` as the single artifact.
2. Briefly note: no picking path in the loop; naïve O(n) if added ad hoc.
3. Open `docs/plan.md` and summarize the enhancement:
   - Add minimal `Ray`/`AABB` and a robust `rayAabbSlab` helper (EPS guards).
   - Document a **uniform grid** broad-phase and a `pickObjectId(ray)` entry.
   - Mark the call site in the loop (comment only in M1).
4. Open `docs/checklist.md` to show M2 verification ideas (two rays, candidate counts).
5. Close with the outcomes mapping (CO3, CO2, CO4, CO5).

---

## Outcomes mapping
- **CO3 (Design/Engineering):** one clear integration point; responsibilities documented in this file.
- **CO2 (Algorithms/Reasoning):** uniform-grid broad-phase + slab narrow phase; numeric stability (EPS).
- **CO4 (Assessment):** simple planned checks next milestone.
- **CO5 (Policies/Docs):** concise in-file comments for assumptions and call site.

---

## Submission checklist (For me)
- [ ] `v-before-review` exists and opens to `MainCode.cpp`.
- [ ] Branch `enhancement/m1-plan` has `docs/plan.md` and `docs/checklist.md`.
- [ ] `README_M1.md` committed on `enhancement/m1-plan`.
- [ ] MP4 recorded following the “What this video will show” steps.
