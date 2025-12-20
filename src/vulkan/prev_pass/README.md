# Overview

Collection of classes for rendering the preview.

## Instanced Render Pass

Background:  


Initially, an instanced render pass might seem like overkill. The primary use
for setting up an instanced render pass is peformance. However, peformance does
not make a lot of sense here. I have not yet started doing peformance testing
on the preview to see what bottlenecks there are. I would also guess that the
fractal preview would take up the majority of the time. Instead, I am
implimenting an instanced render pass to make the preview code simpler. Some of
the biggest complexities for implimenting an instanced render pass is setting
the buffers for multiple different types of materials and switching between the
different materials in the shader. As it turns out, I already had to impliment
both of these for the DE render pass. Not using an instanced render pass
actually makes the code more complicated since I need to have both a bundled
buffer for the DE render pass and individual buffers for the mesh based
objects. Switching entirely over to instanced will make the the buffers more
streamlined accross all the renderable objects.
