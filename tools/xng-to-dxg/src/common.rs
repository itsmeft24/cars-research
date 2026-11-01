use binrw::{BinRead, BinWrite};

#[derive(BinRead, BinWrite, Debug)]
pub struct MeshName {
    #[br(count = 64)]
    name: Vec<u8>,
}

#[derive(BinRead, BinWrite, Debug)]
pub struct Bone {
    name: [u8; 128],
    matrix: [f32; 16],
    bounding_box_center: [f32; 3],
    bounding_box_half: [f32; 3],
    bounding_box_radius: f32,
    parent_index: u32,
}

#[derive(Default, BinRead, BinWrite, Debug)]
pub struct Vector4 {
    pub x: f32,
    pub y: f32,
    pub z: f32,
    pub w: f32,
}

#[derive(Default, BinRead, BinWrite, Debug, Clone, Copy)]
pub struct Vector3 {
    pub x: f32,
    pub y: f32,
    pub z: f32,
}

#[derive(Default, BinRead, BinWrite, Debug)]
pub struct Vector2 {
    pub x: f32,
    pub y: f32,
}

#[derive(Default, BinRead, BinWrite, Debug)]
pub struct MatrixPalette {
    pub num_matrix_ids: u32,
    #[br(count = num_matrix_ids)]
    pub matrix_ids: Vec<u8>,
}

#[derive(Default, BinRead, BinWrite, Debug)]
#[brw(import{num_surfaces: u32})]
pub struct ComplexMatrixPalettes {
    pub matrix_count: u32,
    #[br(count = matrix_count & 0xFFFF)]
    pub matrix_palettes: Vec<MatrixPalette>,
    #[br(if (num_surfaces > 1 && (matrix_count & 0xFFFF) > 0), count = num_surfaces)]
    pub surface_to_matrix_palette_index: Vec<u8>,
}

#[derive(Default, BinRead, BinWrite, Debug)]
pub struct SimpleMatrixPalettes {
    pub marker: u32,
    pub matrix_palette: MatrixPalette,
}