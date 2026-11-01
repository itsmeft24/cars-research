use binrw::{BinRead, BinResult, BinWrite, Endian, binrw};
use std::io::{Seek, Write};

use crate::common::*;

// https://github.com/leeao/carsraceorama/blob/master/carsraceorama/CarsTypes.h#L43
const D3DVSDT_FLOAT1: u8 = 0x12; // 1D float expanded to (value, 0., 0., 1.)
const D3DVSDT_FLOAT2: u8 = 0x22; // 2D float expanded to (value, value, 0., 1.)
const D3DVSDT_FLOAT3: u8 = 0x32; // 3D float expanded to (value, value, value, 1.)
const D3DVSDT_FLOAT4: u8 = 0x42; // 4D float
const D3DVSDT_D3DCOLOR: u8 = 0x40; // 4D packed unsigned bytes mapped to 0. to 1. range
                                   // Input is in D3DCOLOR format (ARGB) expanded to (R, G, B, A)
const D3DVSDT_SHORT2: u8 = 0x25; // 2D signed short expanded to (value, value, 0., 1.)
const D3DVSDT_SHORT4: u8 = 0x45; // 4D signed short

// The following are Xbox extensions
const D3DVSDT_NORMSHORT1: u8 = 0x11; // 1D signed, normalized short expanded to (value, 0, 0., 1.)

// (signed, normalized short maps from -1.0 to 1.0)
const D3DVSDT_NORMSHORT2: u8 = 0x21; // 2D signed, normalized short expanded to (value, value, 0., 1.)
const D3DVSDT_NORMSHORT3: u8 = 0x31; // 3D signed, normalized short expanded to (value, value, value, 1.)
const D3DVSDT_NORMSHORT4: u8 = 0x41; // 4D signed, normalized short expanded to (value, value, value, value)
const D3DVSDT_NORMPACKED3: u8 = 0x16; // 3 signed, normalized components packed in 32-bits.  (11,11,10).

// Each component ranges from -1.0 to 1.0.
// Expanded to (value, value, value, 1.)
const D3DVSDT_SHORT1: u8 = 0x15; // 1D signed short expanded to (value, 0., 0., 1.)

// Signed shorts map to the range [-32768, 32767]
const D3DVSDT_SHORT3: u8 = 0x35; // 3D signed short expanded to (value, value, value, 1.)
const D3DVSDT_PBYTE1: u8 = 0x14; // 1D packed byte expanded to (value, 0., 0., 1.)

// Packed bytes map to the range [0, 1]
const D3DVSDT_PBYTE2: u8 = 0x24; // 2D packed byte expanded to (value, value, 0., 1.)
const D3DVSDT_PBYTE3: u8 = 0x34; // 3D packed byte expanded to (value, value, value, 1.)
const D3DVSDT_PBYTE4: u8 = 0x44; // 4D packed byte expanded to (value, value, value, value)
const D3DVSDT_FLOAT2H: u8 = 0x72; // 2D homogeneous float expanded to (value, value,0., value.)

// Useful for projective texture coordinates.
const D3DVSDT_NONE: u8 = 0x02; // No stream data

const DEFAULT_VERTEX_FORMATS: [u8; 16] = [
    D3DVSDT_FLOAT3,
    D3DVSDT_FLOAT3,
    D3DVSDT_D3DCOLOR,
    D3DVSDT_FLOAT2,
    D3DVSDT_FLOAT1,
    D3DVSDT_NONE,
    D3DVSDT_NONE,
    D3DVSDT_NONE,
    D3DVSDT_FLOAT4,
    D3DVSDT_FLOAT4,
    D3DVSDT_FLOAT2,
    D3DVSDT_NONE,
    D3DVSDT_NONE,
    D3DVSDT_NONE,
    D3DVSDT_NONE,
    D3DVSDT_NONE,
];

#[derive(BinRead, BinWrite, Debug)]
#[brw(little)]
pub struct StreamingDXGMesh {
    pub surface_index: u32,
    pub vertex_type: u32,

    #[br(if ((vertex_type & 0x2000) == 0x2000))]
    pub compression_stuff: Option<[f32; 8]>,

    // #[br(if ((vertex_type & 0x2000) == 0x2000))]
    // pub vertex_formats: Option<[u8; 16]>,
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

    #[br(if ((vertex_type & 0x01) == 0x01), count = num_vertices)]
    pub positions: Option<Vec<Vector3>>,
    #[br(if ((vertex_type & 0x02) == 0x02), count = num_vertices)]
    pub normals: Option<Vec<Vector3>>,
    #[br(if ((vertex_type & 0x08) == 0x08), count = num_vertices)]
    pub colors: Option<Vec<u32>>,
    #[br(if ((vertex_type & 0x04) == 0x04), count = num_vertices)]
    pub uvs: Option<Vec<Vector2>>,
    #[br(if ((vertex_type & 0x10) == 0x10), count = num_vertices)]
    pub uv2: Option<Vec<Vector2>>,
    #[br(if ((vertex_type & 0x40) == 0x40), count = num_vertices)]
    pub single_bone_weights: Option<Vec<f32>>,
    #[br(if ((vertex_type & 0x1000) == 0x1000), count = num_vertices)]
    pub bone_indices: Option<Vec<Vector4>>,
    #[br(if ((vertex_type & 0x1000) == 0x1000), count = num_vertices)]
    pub bone_weights: Option<Vec<Vector4>>,
}

#[derive(BinRead, BinWrite, Debug)]
#[brw(little)]
pub struct DXGLod {
    pub auto_lod_value: f32,
    pub num_meshes: u32,

    #[br(count = num_meshes)]
    pub meshes: Vec<StreamingDXGMesh>,
}

#[binrw]
#[brw(little)]
#[brw(magic = b"dxg\0")]
pub struct DXGHeader {
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
    pub lods: Vec<DXGLod>,

    #[br(calc = lods.iter().map(|lod| lod.num_meshes).sum())]
    #[bw(ignore)]
    pub num_surfaces: u32,

    #[br(if (num_bones >= 29), args{ num_surfaces })]
    #[bw(args{ num_surfaces: *num_surfaces })]
    pub complex_matrix_palettes: Option<ComplexMatrixPalettes>,
    #[br(if (num_bones > 1 && num_bones < 29))]
    pub simple_matrix_palettes: Option<SimpleMatrixPalettes>,
}