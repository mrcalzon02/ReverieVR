# Development image and video-preview publication contract

**Status: default for new generated visual material in ReverieVR.** The image is not considered delivered merely because it exists in a conversation, a temporary file, or a Google Drive archive. Preserve the full-resolution originals in the shared art archive when applicable, **and publish reviewable assets in GitHub under a project gallery**.

## Required steps for every visual-generation pass

1. **Produce the requested visual only**, and label it accurately: actual game capture, Blender/OBJ render, image-generation concept art, externally licensed reference, or mockup. Never label concept art as a game screenshot or a rough model as installed content.
2. **Source and preserve originals.** Keep the authoring file/generator/source prompt where permitted, descriptive naming, source revision, rights status, and unmodified masters. Archive large originals in Drive only as a supplement; don't rely on a private Drive image for the GitHub gallery preview.
3. **Publish a GitHub gallery entry in the affected repository** at `docs/gallery/<project>/<collection>/<batch>/` with images/GIFs that actually display on GitHub; keep one stable `README.md` and `gallery.json`. Create/maintain `docs/gallery/README.md` as the discoverable index. Place both multi-item contact sheets and each individual requested turntable/scene in the gallery. New image types must be supported by the verifier before publication.
4. **Record provenance** in `gallery.json`: a full source commit SHA, source_kind, licensing/rights status, file bytes, SHA-256, and (for animated GIFs) dimensions/frame count. Name what is unfinished. If a design was generated from uploaded user references, describe that truthfully without suggesting it is publicly licensed by default.
5. **Stage, verify, commit and push to `main`** without force pushes. Run `python3 scripts/gallery/verify_generated_gallery.py`, plus project-specific source/model tests. Wait for CI, check GitHub remote head and image files, and provide the user the exact commit link plus a clickable gallery link. Never claim upload/publishing before readback.
6. **Keep development record linked** in the project's main docs and, for major visual updates, the repository README; log the next model improvement targets and whether the assets are implemented in the game.

To prepare new content from a local image-generation folder:

```sh
python3 scripts/gallery/publish_generated_images.py \
  --project sitka --collection shoreline --batch 2026-11-05 \
  --source-dir /path/to/reviewed-images \
  --title 'Sitka shore specimen review' \
  --source-commit <exact-40-character-commit-sha> \
  --license 'Project-origin review; rights verification pending' \
  --source-kind 'Original low-poly source mesh render'
python3 scripts/gallery/verify_generated_gallery.py
# Then commit the actual images, README and gallery manifest to main.
```

The publisher **stages files locally**, updates the gallery index, refuses overwriting existing batches, and validates the media. It does not have user Git credentials and **cannot itself promise a remote push**. The agent or CI publishing mechanism must perform the GitHub commit and verify it afterwards. For large turntables, publish optimized 24-frame web previews in GitHub while retaining original renders and OBJ sources in the archive; explicitly link the master archive. Do not replace an image with a link to something that was never uploaded.

The checked-in GitHub Actions **Gallery image integrity** job validates every gallery asset against source metadata and checks that all assets appear in the README and the gallery appears in the root index. A failed image integrity check must be reported, not silently bypassed.

## Scope and exclusions

The contract applies to new generated images intended as **project development assets**: models, props, terrain sheets, game captures, promotional artwork, and approved concept references. Do not publish confidential user uploads, family/personal images, third-party works without rights, private location data, or unrelated speculative imagery. Repositories that do not yet have this contract must adopt its gallery schema and verifiers as part of their next visual-content change; this file cannot silently alter other repos.