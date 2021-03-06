(local size 128)
(self.shader 'fragment "shader/terrain.frag")
(self.shader 'vertex "shader/terrain.vert")
(self.vertex_count (* size size 6))
(self.texture 'height_tex (+ ".stb_perlin?type=ridge&span=3&octaves=5&lacunarity=2.3&gain=0.35&size=" size))
(self.texture 'color_tex ".stb_perlin?type=turbulence&octaves=4&span=8&wrap=8&size=1024")
(self.scalar 'size size)
