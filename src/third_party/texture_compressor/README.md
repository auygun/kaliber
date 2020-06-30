Real-time texture compression code salvaged from chromium project repository.
Implements ATC, DXT and ETC1 compression, optimized for NEON.  

It was used in chromium project for compositor tiles to reduce the GPU memory
usage for low to mid-end mobile devices. It got obsolete with the GPU rasterizer
introduced in chromium 37 and removed.
