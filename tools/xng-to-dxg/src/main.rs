use binrw::{BinRead, BinWrite};
use std::{env, path::PathBuf};

mod common;
mod dxg;
mod xng;

fn main() {
    let xng_path = PathBuf::from(env::args().nth(1).unwrap());
    let dxg_path = PathBuf::from(env::args().nth(2).unwrap());
    
    let mut xng_file = std::fs::File::open(xng_path).unwrap();
    let xng = xng::XNGHeader::read(&mut xng_file).unwrap();

    let mut dxg_lods = Vec::new();
    for lod in xng.lods {
        let mut dxg_meshes = Vec::new();
        for mesh in lod.meshes {
            let mut vertex_type = mesh.vertex_type;
            // remove tangent, delta_cpu, compression, uv3, and uv4
            if (vertex_type & 0x20) == 0x20 {
                vertex_type &= !0x20;
            }
            if (vertex_type & 0x100) == 0x100 {
                vertex_type &= !0x100;
            }
            if (vertex_type & 0x2000) == 0x2000 {
                panic!("input XNG uses compression, which is currently not implemented sorri");
                vertex_type &= !0x2000;
            }
            if (vertex_type & 0x4000) == 0x4000 {
                vertex_type &= !0x4000;
            }
            if (vertex_type & 0x8000) == 0x8000 {
                vertex_type &= !0x8000;
            }

            let colors = if let Some(colors) = mesh.colors {
                let mut swapped_colors = colors.clone();
                swapped_colors.iter_mut().map(|pixel| ((*pixel & 0xFF) << 16) | ((*pixel & 0xFF00) >> 8) | ((*pixel & 0xFF0000) << 8) | (*pixel & 0xFF000000)).count();
                Some(swapped_colors)
            } else {
                None
            };
            let dxg_mesh = dxg::StreamingDXGMesh {
                surface_index: mesh.surface_index,
                vertex_type,
                compression_stuff: mesh.compression_stuff,
                num_texture_coordinate_sets: mesh.num_texture_coordinate_sets,
                compressed: mesh.compressed,
                streaming: mesh.streaming,
                unk: mesh.unk,
                unk2: mesh.unk2,
                unk3: mesh.unk3,
                texture_coordinate_sets: mesh.texture_coordinate_sets,
                num_vertices: mesh.num_vertices,
                num_face_indices: mesh.num_face_indices,
                indices: mesh.indices,
                positions: mesh.positions,
                normals: mesh.normals,
                colors,
                uvs: mesh.uvs,
                uv2: mesh.uv2,
                single_bone_weights: mesh.single_bone_weights,
                bone_indices: mesh.bone_indices,
                bone_weights: mesh.bone_weights,
            };
            dxg_meshes.push(dxg_mesh);
        }
        let dxg_lod = dxg::DXGLod {
            auto_lod_value: lod.auto_lod_value,
            num_meshes: lod.num_meshes,
            meshes: dxg_meshes
        };
        dxg_lods.push(dxg_lod);
    }
    let dxg = dxg::DXGHeader {
        version: xng.version,
        num_bones: xng.num_bones,
        bones: xng.bones,
        num_mesh_names: xng.num_mesh_names,
        mesh_names: xng.mesh_names,
        num_lod: xng.num_lod,
        skin_animates_flag: xng.skin_animates_flag,
        has_weight: xng.has_weight,
        unused: xng.unused,
        lods: dxg_lods,
        num_surfaces: xng.num_surfaces,
        complex_matrix_palettes: xng.complex_matrix_palettes,
        simple_matrix_palettes: xng.simple_matrix_palettes,
    };
    
    let mut dxg_file = std::fs::File::create(dxg_path).unwrap();
    dxg.write(&mut dxg_file).unwrap();
}
