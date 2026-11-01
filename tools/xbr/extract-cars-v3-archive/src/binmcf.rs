use std::{
    io::{Read, Write},
    path::Path,
};

use binrw::*;
use flate2::read::ZlibDecoder;
use reqwest::header::Entry;

use crate::{bpb, utils};

#[binread]
#[br(big)]
#[br(magic = b"BINMCF\x00\xCC")]
#[derive(Debug)]
pub struct MotionControlPackHeader {
    pub motion_control_file_offset: u32,
    pub motion_control_file_len: u32,
    pub table_offset: u32,
    pub motion_count: u32,
    pub string_table_offset: u32,
    pub string_table_len: u32,
}

#[binread]
#[br(big)]
#[derive(Debug)]

pub struct MotionEntry {
    pub offset: u32,
    pub compressed_len: u32,
    pub decompressed_len: u32,
    pub name_offset: u32,
}

pub fn extract(name: &str, buffer: &[u8], out_dir: &Path) -> Option<()> {
    let mut cursor = std::io::Cursor::new(buffer);
    let file_header = MotionControlPackHeader::read_be(&mut cursor).ok()?;
    // Convert the inner .mcf file from .bpb to plaintext ParameterBlock format.
    let motion_control_file_path = out_dir.join(name.replace(".x360.bin", ""));
    println!("Extracting file: {}", name.replace(".x360.bin", ""));
    bpb::convert(
        &buffer[file_header.motion_control_file_offset as usize
            ..file_header.motion_control_file_offset as usize
                + file_header.motion_control_file_len as usize],
        &motion_control_file_path,
    )
    .unwrap();

    cursor.set_position(file_header.table_offset as u64);
    let entries = Vec::<MotionEntry>::read_options(
        &mut cursor,
        Endian::Big,
        VecArgs::builder()
            .count(file_header.motion_count as usize)
            .finalize(),
    )
    .ok()?;
    for entry in entries.iter() {
        if entry.compressed_len == 0 {
            continue;
        }
        let path = utils::read_string(
            &buffer[file_header.string_table_offset as usize + entry.name_offset as usize
                ..buffer.len()],
        );
        println!("Extracting file: {}", path);
        let motion_path = out_dir.join(path);

        let mut decompressed = Vec::with_capacity(entry.decompressed_len as usize);
        let mut decompressor = ZlibDecoder::new(
            &buffer[entry.offset as usize..entry.offset as usize + entry.compressed_len as usize],
        );
        decompressor.read_to_end(&mut decompressed).unwrap();

        if let Some(parent) = motion_path.parent() {
            std::fs::create_dir_all(parent).unwrap();
        }
        let mut out = std::fs::File::create(motion_path).unwrap();
        out.write(&decompressed).unwrap();
    }

    None
}
