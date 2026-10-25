# Batching

This sample compares separate draw calls with a simple static geometry batch.
Both sides contain the same 36 quads:

- The orange quads on the left reuse one small vertex buffer and are positioned
  with a uniform. The CPU submits one draw call per quad.
- The blue quads on the right have their transformed vertices stored
  consecutively in one vertex buffer. The CPU submits the entire batch with one
  draw call.

The comparison keeps the shader and render state compatible on both sides so
that the draw-call organization is the important difference. The geometry
uses 37 draw calls per frame: 36 for the deliberately unbatched side and one
for the batched side. The counts appear below their respective groups and
exclude the additional draw calls used by the text labels and help overlay.

| Key | Action |
| --- | --- |
| `H` | Show / hide keyboard help |
| `Esc` | Quit |

The example bakes the batched positions once on the CPU to keep the data flow
visible in `src/main.cpp`. A production renderer may instead rebuild dynamic
batches, use indices, instancing, multi-draw commands, or GPU-driven rendering.
Larger batches reduce submission overhead but can also reduce culling
granularity and make order-dependent rendering harder.
