use binrw::*;

#[binrw]
pub struct Header {
    pub version: u32,
    pub file_count: u32,
    pub unk: u32,
    pub unk1: u32,
    pub table_size: u32,
    pub padding: u32,
}

#[binrw]
#[derive(Debug)]
pub struct FileEntry {
    pub hash: u32,
    pub len: u32,
    offset: u32,
}

impl FileEntry {
    pub fn offset(&self) -> usize {
        (self.offset as usize) << 11
    }
}

#[binrw]
#[brw(big)]
pub struct RainbowArchive {
    pub header: Header,
    #[br(count = header.file_count)]
    pub files: Vec<FileEntry>,
}
