pub struct TextureExtents {
    pub width: u32,
    pub height: u32,
    pub depth: u32,
    pub array_layers: u32,
    pub mip_count: u32,
    pub is_cubemap: bool,
}

impl TextureExtents {
    pub fn print_info(&self) {
        println!("size: {}x{}x{} array_layers: {} mip_count: {} is_cubemap: {}", self.width, self.height, self.depth, self.array_layers, self.mip_count, self.is_cubemap);
    }
}