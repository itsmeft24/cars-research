// #![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use std::{
    collections::HashMap,
    io::{Read, Seek, Write},
    path::Path,
    time::SystemTime,
};

use aud_hashes::AUDIO_HASHES;
use binrw::*;
use mus_hashes::MUSIC_HASHES;
use xbr::RainbowArchive;
mod aud_hashes;
mod binmcf;
mod bpb;
mod mus_hashes;
mod utils;
mod xbr;

const HASH_LABEL_DB_URL: &str = "https://raw.githubusercontent.com/itsmeft24/cars-research/refs/heads/main/hash_labels/xbr_hashes.txt";
const DEFAULT_HASH_LABEL_DB: &str = include_str!("../../../../hash_labels/xbr_hashes.txt");

fn download_labels() -> Result<String, reqwest::Error> {
    let response = reqwest::blocking::get(HASH_LABEL_DB_URL)?;
    let body = response.text()?;
    Ok(body)
}

fn extract_main(install_dir: &Path) {
    let labels = download_labels().unwrap_or(DEFAULT_HASH_LABEL_DB.to_string());
    let hashes: Vec<&str> = labels.split("\n").collect();
    let mut map = HashMap::with_capacity(hashes.len());
    for string in hashes {
        map.insert(utils::djb_hash(string), string.to_string());
    }

    let mut file = std::fs::File::open("Cars3.Xbr").unwrap();
    if file.metadata().unwrap().len() != 1263589376 {
        panic!("File size does not match NTSC-U Cars3.Xbr!");
    }

    let archive = RainbowArchive::read(&mut file).unwrap();
    
    // Get the largest file's size and create a work buffer for extracting files.
    let work_buffer_len = archive.files.iter().max_by_key(|f| f.len).unwrap().len as usize;
    let mut work_buffer = vec![0u8; work_buffer_len];

    for entry in archive.files.iter() {
        if let Some(name) = map.get(&entry.hash) {
            // The game's .mst.bin files are not necessary, since their unpacked counterparts are also present in the archive.
            // Since we're shipping the main executable alongside the new 5.0 shader model shaders, we can skip dumping the Xenos ones.
            if name.ends_with(".mst.x360.bin") || name.ends_with(".vsh") || name.ends_with(".psh") {
                continue;
            }

            // Read the file contents into the work buffer.
            file.seek(std::io::SeekFrom::Start(entry.offset() as u64))
                .unwrap();
            file.read(&mut work_buffer[0..entry.len as usize]).unwrap();

            // Create the output file path.
            let out_path = install_dir.join(name);
            if let Some(parent) = out_path.parent() {
                std::fs::create_dir_all(parent).unwrap();
            }

            // If we encounter a .bpb, we want to convert it to a plaintext ParameterBlock.
            if out_path.extension().unwrap() == "bpb" {
                // Strips away the last extension.
                let new_path = if let Some(parent) = out_path.parent() {
                    parent.join(out_path.file_stem().unwrap())
                } else {
                    Path::new(out_path.file_stem().unwrap()).to_path_buf()
                };

                println!("Extracting file: {}", name.replace(".bpb", ""));

                bpb::convert(&work_buffer[0..entry.len as usize], &new_path).unwrap();
            } else if name.ends_with(".mcf.x360.bin") {
                binmcf::extract(name, &work_buffer[0..entry.len as usize], install_dir);
            } else {
                println!("Extracting file: {}", name);
                // Dumps the contents of the work buffer to the destination file.
                let mut out = std::fs::File::create(out_path).unwrap();
                out.write(&work_buffer[0..entry.len as usize]).unwrap();
            }
        }
    }
}

fn extract_music(install_dir: &Path) {
    let mut file = std::fs::File::open("Cars3_Mus.Xbr").unwrap();
    if file.metadata().unwrap().len() != 79775744 {
        panic!("File size does not match NTSC-U Cars3_Mus.Xbr!");
    }

    let archive = RainbowArchive::read(&mut file).unwrap();
    if archive.files.len() != MUSIC_HASHES.len() {
        panic!("File count does not match NTSC-U Cars3_Mus.Xbr!");
    }
    // Get the largest file's size and create a work buffer for extracting files.
    let work_buffer_len = archive.files.iter().max_by_key(|f| f.len).unwrap().len as usize;
    let mut work_buffer = vec![0u8; work_buffer_len];

    for (i, entry) in archive.files.iter().enumerate() {
        let name = MUSIC_HASHES[i];
        // Read the file contents into the work buffer.
        file.seek(std::io::SeekFrom::Start(entry.offset() as u64))
            .unwrap();
        file.read(&mut work_buffer[0..entry.len as usize]).unwrap();

        // Create the output file path.
        let out_path = install_dir.join(name);
        if let Some(parent) = out_path.parent() {
            std::fs::create_dir_all(parent).unwrap();
        }
        println!("Extracting file: {}", name);
        // Dumps the contents of the work buffer to the destination file.
        let mut out = std::fs::File::create(out_path).unwrap();
        out.write(&work_buffer[0..entry.len as usize]).unwrap();
    }
}

fn extract_audio(install_dir: &Path) {
    let mut file = std::fs::File::open("Cars3_Aud.Xbr").unwrap();
    if file.metadata().unwrap().len() != 559953920 {
        panic!("File size does not match NTSC-U Cars3_Aud.Xbr!");
    }

    let archive = RainbowArchive::read(&mut file).unwrap();
    if archive.files.len() != AUDIO_HASHES.len() {
        panic!("File count does not match NTSC-U Cars3_Aud.Xbr!");
    }
    // Get the largest file's size and create a work buffer for extracting files.
    let work_buffer_len = archive.files.iter().max_by_key(|f| f.len).unwrap().len as usize;
    let mut work_buffer = vec![0u8; work_buffer_len];

    for (i, entry) in archive.files.iter().enumerate() {
        let name = AUDIO_HASHES[i];
        // Read the file contents into the work buffer.
        file.seek(std::io::SeekFrom::Start(entry.offset() as u64))
            .unwrap();
        file.read(&mut work_buffer[0..entry.len as usize]).unwrap();

        // Create the output file path.
        let out_path = install_dir.join(name);
        if let Some(parent) = out_path.parent() {
            std::fs::create_dir_all(parent).unwrap();
        }
        println!("Extracting file: {}", name);
        // Dumps the contents of the work buffer to the destination file.
        let mut out = std::fs::File::create(out_path).unwrap();
        out.write(&work_buffer[0..entry.len as usize]).unwrap();
    }
}

fn main() {
    let install_dir = std::path::Path::new("./install/");
    let start = SystemTime::now();
    extract_main(install_dir);
    extract_music(install_dir);
    extract_audio(install_dir);
    let elapsed = start.elapsed().unwrap().as_secs_f64();
    println!("Finished installing in {} seconds!", elapsed);
}
