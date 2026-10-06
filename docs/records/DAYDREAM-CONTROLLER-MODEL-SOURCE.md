# Daydream controller model source

Status: approved source reference; original asset not yet vendored.

## Primary source

Model: **Daydream Vr Controller**  
Author: **rje**  
Sketchfab model ID: `c1944c64e06544babc90e9d0aa953551`  
Source: https://sketchfab.com/3d-models/daydream-vr-controller-c1944c64e06544babc90e9d0aa953551

Sketchfab currently describes the source model as:

- downloadable;
- 686 triangles;
- 709 vertices;
- licensed under Creative Commons Attribution (CC BY);
- intended as a simple Daydream-controller mesh for apps/prototypes.

When the original source asset is vendored into ReverieVR, retain the author attribution and source URL with the asset and in third-party notices.

## Derivative source deliberately not vendored

A modified OBJ/MTL/texture derivative exists at:

https://github.com/TechnoBuddhist/VR-Controller-Daydream

Its README says it was created from rje's Sketchfab model and may be used with attribution, but that repository also carries a GPL-3.0 repository license.

Do not vendor that derivative into ReverieVR without an explicit licensing decision. Prefer the original CC-BY Sketchfab source supplied by the project owner.

## Intended ReverieVR use

The Daydream controller is a 3DoF orientation-tracked device, not a positionally tracked 6DoF controller.

ReverieVR therefore renders the controller at a fixed virtual hand-origin offset and applies only the real controller orientation to the mesh. The pointer ray originates from the rendered model and uses the same quaternion-derived forward direction as UI hit testing.

Do not infer or advertise positional tracking.

## Current implementation state

The controller pointer/raycast system is implemented independently of the visual mesh:

- selectable Auto / Gaze / Controller pointer modes;
- fresh physical-controller pose drives the controller ray in Auto/Controller modes;
- gaze fallback is used in Auto when controller pose is unavailable/stale;
- UI raycasts hit the actual world-space menu planes;
- visible ray endpoint matches the same hit-test distance;
- focus, activation, and failure use distinct feedback states.

Once the original CC-BY model bytes are available in repository-safe form, the mesh can replace the invisible fixed controller origin without changing input semantics.
