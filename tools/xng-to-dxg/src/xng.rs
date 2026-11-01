use binrw::{BinRead, BinResult, BinWrite, Endian, binrw};
use std::io::{Seek, Write};

use crate::common::*;

#[derive(Default, BinRead, BinWrite, Debug)]
#[brw(big)]
pub struct XNGDeltaBlock {
    num_channels: u32,

    #[br(count = 64)]
    controller_name: Vec<u8>,

    num_vertices: u32,
    xyz_bits: u32,
    force_unique: u8,
    unk: u32,
    unk2: u32,

    delta_count: u32,

    #[br(count = delta_count)]
    delta_positions: Vec<Vector4>,

    #[br(count = delta_count)]
    delta_normals: Vec<Vector4>,

    #[br(count = delta_count)]
    delta_indices: Vec<i32>,

    #[br(count = num_vertices)]
    positions: Vec<Vector4>,

    #[br(count = num_vertices)]
    normals: Vec<Vector4>,
}

#[derive(BinRead, BinWrite, Debug)]
#[brw(big)]
pub struct StreamingXNGMesh {
    pub surface_index: u32,
    pub vertex_type: u32,

    #[br(if ((vertex_type & 0x2000) == 0x2000))]
    pub compression_stuff: Option<[f32; 8]>,

    pub num_texture_coordinate_sets: u8,
    pub compressed: u8,
    pub streaming: u8,
    pub unk: u8,
    pub unk2: u8,
    pub unk3: u8,

    #[br(count = num_texture_coordinate_sets)]
    pub texture_coordinate_sets: Vec<f32>,

    pub num_vertices: u16,
    pub num_face_indices: u16,

    #[br(count = num_face_indices)]
    pub indices: Vec<u16>,

    #[br(if (num_face_indices % 2 == 1))]
    pub unk_flag: Option<u16>,

    #[br(if ((vertex_type & 0x01) == 0x01), count = num_vertices)]
    pub positions: Option<Vec<Vector3>>,
    #[br(if ((vertex_type & 0x02) == 0x02), count = num_vertices)]
    pub normals: Option<Vec<Vector3>>,
    #[br(if ((vertex_type & 0x08) == 0x08), count = num_vertices)]
    pub colors: Option<Vec<u32>>,
    #[br(if ((vertex_type & 0x04) == 0x04), count = num_vertices)]
    pub uvs: Option<Vec<Vector2>>,
    #[br(if ((vertex_type & 0x40) == 0x40), count = num_vertices)]
    pub single_bone_weights: Option<Vec<f32>>,
    #[br(if ((vertex_type & 0x1000) == 0x1000), count = num_vertices)]
    pub bone_indices: Option<Vec<Vector4>>,
    #[br(if ((vertex_type & 0x1000) == 0x1000), count = num_vertices)]
    pub bone_weights: Option<Vec<Vector4>>,
    #[br(if ((vertex_type & 0x10) == 0x10), count = num_vertices)]
    pub uv2: Option<Vec<Vector2>>,
    #[br(if ((vertex_type & 0x4000) == 0x4000), count = num_vertices)]
    pub uv3: Option<Vec<Vector2>>,
    #[br(if ((vertex_type & 0x8000) == 0x8000), count = num_vertices)]
    pub uv4: Option<Vec<Vector2>>,
    #[br(if ((vertex_type & 0x20) == 0x20), count = num_vertices)]
    pub tangents: Option<Vec<Vector3>>,

    #[br(if ((vertex_type & 0x100) == 0x100))]
    pub delta_block: Option<XNGDeltaBlock>,
}

#[derive(BinRead, BinWrite, Debug)]
#[brw(big)]
pub struct XNGLod {
    pub auto_lod_value: f32,
    pub num_meshes: u32,

    #[br(count = num_meshes)]
    pub meshes: Vec<StreamingXNGMesh>,
}

#[binrw]
#[brw(big)]
#[brw(magic = b"xng\0")]
pub struct XNGHeader {
    pub version: i32,
    pub num_bones: u32,

    #[br(count = num_bones)]
    pub bones: Vec<Bone>,

    pub num_mesh_names: i32,

    #[br(count = num_mesh_names)]
    pub mesh_names: Vec<MeshName>,

    pub num_lod: u8,
    pub skin_animates_flag: u8,
    pub has_weight: u8,
    pub unused: u8,

    #[br(count = num_lod)]
    pub lods: Vec<XNGLod>,

    #[br(calc = lods.iter().map(|lod| lod.num_meshes).sum())]
    #[bw(ignore)]
    pub num_surfaces: u32,

    #[br(if (num_bones >= 29), args{ num_surfaces })]
    #[bw(args{ num_surfaces: *num_surfaces })]
    pub complex_matrix_palettes: Option<ComplexMatrixPalettes>,
    #[br(if (num_bones > 1 && num_bones < 29))]
    pub simple_matrix_palettes: Option<SimpleMatrixPalettes>,
}
