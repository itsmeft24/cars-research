use std::fs::File;
use std::io::{self, Write};
use std::path::Path;

use binrw::{BinRead, binread};

use crate::utils::{read_string, unzlibme};

#[binread]
#[br(big)]
#[derive(Debug)]
struct FileHeader {
    pub magic: [u8; 4],
    pub headers_offset: u32,
    pub parameters_offset: u32,
    pub header_count: u16,
    pub parameter_count: u16,
    pub label_string_table_offset: u32,
    pub label_string_table_len: u32,
    pub value_string_table_offset: u32,
    pub value_string_table_len: u32,

    #[br(count = header_count)]
    pub headers: Vec<Header>,

    #[br(count = parameter_count)]
    pub parameters: Vec<Parameter>,
}

#[binread]
#[br(big)]
#[derive(Debug)]
struct Header {
    pub hash: u32,
    pub string_table_offset: u32,
    pub flag: u16,
    pub parameter_count: u16,
    pub parameter_start_index: u32,
}

enum Value {
    StringTableOffset(usize),
    InlineString(String),
    Integer(i32),
    Float(f32),
}

#[binread]
#[br(big)]
#[derive(Debug)]
struct Parameter {
    pub hash: u32,
    string_table_offset_bitfield: u32,
    pub flag: u16,
    pub reserved: [u8; 6],
}

impl Parameter {
    pub fn string_table_offset(&self) -> usize {
        (self.string_table_offset_bitfield & 0x3FFFFFFF) as usize
    }
    pub fn value(&self) -> Value {
        match self.string_table_offset_bitfield >> 30 {
            0 => Value::StringTableOffset(u32::from_be_bytes(
                self.reserved[2..6].try_into().unwrap(),
            ) as usize),
            1 => Value::InlineString(read_string(&self.reserved)),
            2 => Value::Integer(i32::from_be_bytes(self.reserved[2..6].try_into().unwrap())),
            3 => Value::Float(f32::from_be_bytes(self.reserved[2..6].try_into().unwrap())),
            _ => panic!("Unimplemented value type!"),
        }
    }
}

pub fn convert(file_data: &[u8], dest_path: &Path) -> Option<()> {
    // If we find a ZLIBME header, we want to extract the inner file and use that. Otherwise we want to fall back to the buffer passed in.
    let data = if let Some(data) = unzlibme(file_data) {
        data
    } else {
        file_data.to_owned()
    };

    let mut cursor = io::Cursor::new(&data);
    let file_header: FileHeader = FileHeader::read_be(&mut cursor).ok()?;

    let mut out = File::create(&dest_path).ok()?;

    for (i, header) in file_header.headers.iter().enumerate() {
        let prefix = read_string(
            &data[file_header.label_string_table_offset as usize
                + header.string_table_offset as usize..data.len()],
        );

        let header_name = if header.flag == 0xFFFF {
            if i != 0 {
                format!("\n[{}]", prefix)
            } else {
                format!("[{}]", prefix)
            }
        } else {
            if i != 0 {
                format!("\n[{}{}]", prefix, header.flag)
            } else {
                format!("[{}{}]", prefix, header.flag)
            }
        };
        writeln!(out, "{}", header_name);

        for parameter in file_header.parameters[header.parameter_start_index as usize
            ..header.parameter_start_index as usize + header.parameter_count as usize]
            .iter()
        {
            let parameter_prefix = read_string(
                &data[file_header.label_string_table_offset as usize
                    + parameter.string_table_offset()..data.len()],
            );
            let parameter_name = if parameter.flag == 0xFFFF {
                format!("{}", parameter_prefix)
            } else {
                format!("{}{}", parameter_prefix, parameter.flag)
            };
            match parameter.value() {
                Value::StringTableOffset(offset) => {
                    let string = read_string(
                        &data[file_header.value_string_table_offset as usize + offset..data.len()],
                    );
                    if string.contains(" ") {
                        writeln!(out, "{}=\"{}\"", parameter_name, string);
                    } else {
                        writeln!(out, "{}={}", parameter_name, string);
                    }
                }
                Value::InlineString(inline_string) => {
                    if inline_string.contains(" ") {
                        writeln!(out, "{}=\"{}\"", parameter_name, inline_string);
                    } else {
                        writeln!(out, "{}={}", parameter_name, inline_string);
                    }
                }
                Value::Integer(integer) => {
                    writeln!(out, "{}={}", parameter_name, integer);
                }
                Value::Float(float) => {
                    writeln!(out, "{}={}", parameter_name, float);
                }
            }
        }
    }
    Some(())
}
