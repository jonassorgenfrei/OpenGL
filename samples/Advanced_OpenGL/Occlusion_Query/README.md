# Occlusion query

This sample demonstrates GPU visibility testing with an occlusion query. An
orange gear with 192 triangles moves behind a nearer gray occluder. When its box
is completely hidden, conditional rendering can skip its real draw.

Each frame follows the relevant operations directly in `src/main.cpp`:

1. Draw the occluder to populate the depth buffer.
2. Disable color and depth writes.
3. Draw the object's bounding rectangle inside a
   `GL_ANY_SAMPLES_PASSED` query.
4. Restore the write masks.
5. Wrap the real object draw in `glBeginConditionalRender` and
   `glEndConditionalRender`.

| Key | Action |
| --- | --- |
| `Esc` | Quit |
| `H` | Show / hide keyboard help |
| `B` | Show / hide the yellow bounding-box outline |

The text below the scene shows **Rendered - potentially visible** in green or
**Not rendered - occluded** in red. It describes the orange object's draw;
the occluder, query proxy and UI still issue their own draws.

This teaching demo uses `GL_QUERY_WAIT` and reads `GL_QUERY_RESULT` to keep the
status synchronized with the current frame's actual render/skip decision.
The readback can stall the CPU while the GPU finishes the query. With
`GL_QUERY_NO_WAIT`, an unavailable result may instead cause conservative
rendering; it would not support the same definitive status display.

The gear has twelve teeth and an open center. Its axis-aligned bounding box is
computed from the mesh vertices and follows the same scale and translation.
The query renders only the filled box (two triangles), while conditional
rendering draws the actual gear. A visible box is conservative: it does not
guarantee that the enclosed mesh has visible samples.

Press `B` to show the box even through the occluder. This debug outline is drawn
after the conditional draw without depth testing or depth writes, so it cannot
change the query result. The query always uses the filled box, even when its
outline is hidden.

Production renderers generally test cheap conservative
bounds, consume older query results to avoid pipeline stalls, and only use
queries when the avoided rendering work is greater than the query overhead.
