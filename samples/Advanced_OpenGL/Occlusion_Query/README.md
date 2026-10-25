# Occlusion query

This sample demonstrates GPU visibility testing with an occlusion query. An
orange object moves horizontally behind a nearer gray occluder. When the object
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

The text below the scene shows **Rendered - object visible** in green or
**Not rendered - occluded** in red. It describes the orange object's draw;
the occluder, query proxy and UI still issue their own draws.

This teaching demo uses `GL_QUERY_WAIT` and reads `GL_QUERY_RESULT` to keep the
status synchronized with the current frame's actual render/skip decision.
The readback can stall the CPU while the GPU finishes the query. With
`GL_QUERY_NO_WAIT`, an unavailable result may instead cause conservative
rendering; it would not support the same definitive status display.

The demonstration uses the object's rectangle as its own bound for clarity.
Production renderers generally test cheap conservative
bounds, consume older query results to avoid pipeline stalls, and only use
queries when the avoided rendering work is greater than the query overhead.
