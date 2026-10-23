# Breakwater left-side 3D weapon switching — 2026-10-23

Scoped native Breakwater combat addition on main. The player has a physical five-slot rack to the left, showing the original mounted-gun meshes scaled into true 3D miniature models for guns already purchased. Unowned slots are visibly locked and have no model. The selected gun and hovered slot have distinct visual states. A finite 3D controller-ray/cuboid hit chooses a slot; an edge-gated Select press invokes the existing Campaign::SelectWeapon so ownership, retained magazines and the reload lock are unchanged. The whole press is captured to prevent switching clicks becoming shots. Activation/rejection feedback routes through existing native feedback. Controller-pad cycling and the old combat HUD remain as alternate methods.

Existing breakwater_gun_mesh_test.cpp was extended for all five miniature mesh vertex bounds, exact ray slot targets and invalid rays. The normal scripts/verify-breakwater-visuals.sh already executes that test; no new test framework or GitHub Actions required.

Acceptance not yet established: Android NDK/GLES module compilation, installed APK, Samsung S9/Daydream physical model visibility, controller pick feel, stereo comfort, perf and day/night UI collision tests. No shared shell/input API, non-Breakwater gameplay or save schema change.
