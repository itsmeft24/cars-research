use flate2::read::ZlibDecoder;
use std::io::Read;

pub fn djb_hash(input: &str) -> u32 {
    let mut hash = 5381u32;
    for byte in input.to_ascii_lowercase().bytes() {
        hash = ((hash << 5).wrapping_add(hash)).wrapping_add(byte as u32);
    }
    hash
}

pub fn read_string(input: &[u8]) -> String {
    let mut output = Vec::new();

    for c in input {
        if c == &0u8 {
            return std::str::from_utf8(&output).unwrap().to_owned();
        }
        output.push(*c)
    }

    std::str::from_utf8(&output).unwrap().to_owned()
}

pub fn unzlibme(file_data: &[u8]) -> Option<Vec<u8>> {
    // Check to make sure we can at least read a ZlibCompressionHeader from the buffer.
    if file_data.len() < 16 {
        return None;
    }

    // If the file starts with 'ZLIBME', we need to grab the compressed and decompressed sizes from the ZlibCompressionHeader and extract the inner data.
    if &file_data[0..6] == b"ZLIBME" {
        let compressed_size = u32::from_be_bytes(file_data[8..12].try_into().unwrap()) as usize;
        let decompressed_size = u32::from_be_bytes(file_data[12..16].try_into().unwrap()) as usize;
        if file_data.len() < compressed_size + 16 {
            return None;
        }
        let mut decompressed = Vec::with_capacity(decompressed_size);
        let mut decompressor = ZlibDecoder::new(&file_data[16..compressed_size + 16]);
        decompressor.read_to_end(&mut decompressed).unwrap();
        return Some(decompressed);
    } else {
        return None;
    }
}
