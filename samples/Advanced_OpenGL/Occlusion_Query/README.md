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

`GL_QUERY_NO_WAIT` prevents the renderer from waiting for an unfinished query.
If the result is not ready, OpenGL draws conservatively, avoiding an incorrect
visibility rejection. The demonstration uses the object's rectangle as its
own bound for clarity. Production renderers generally test cheap conservative
bounds, consume older query results to avoid pipeline stalls, and only use
queries when the avoided rendering work is greater than the query overhead.
