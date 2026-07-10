use num_traits::cast::FromPrimitive;
use std::io::Cursor;
use anyhow::Result;
use binrw::BinRead;
use byteorder::{LittleEndian, ByteOrder, ReadBytesExt, BigEndian};
use ddsfile::DxgiFormat;
use vulkanite::vk;
use crate::dxgi_format_map::map_dxgi_format_to_vk;
use crate::textureinfo::TextureExtents;


const DDS_MAGIC: u32 = 0x20534444;

const FLAG_DDSD_PITCH: u32 = 0x8;
const FLAG_DDSD_MIPMAPCOUNT: u32 = 0x20000;
const FLAG_DDSD_LINEARSIZE: u32 = 0x80000;
const FLAG_DDSD_DEPTH: u32 = 0x800000;

const FLAG_DDSCAPS2_CUBEMAP: u32 = 0x200;

const FLAGS_ALL_CUBEMAP_FACES: u32 = 0x400 | 0x800 | 0x1000 | 0x2000 | 0x4000 | 0x8000;

pub struct DdsFile<'a> {
    pub magic: u32,
    pub header: DdsHeader,
    pub dxt10: Option<DdsHeaderDXT10>,
    pub data: &'a [u8],
}

#[derive(BinRead)]
#[brw(little)]
pub struct DdsHeader {
    pub size: u32,
    pub flags: u32,
    pub height: u32,
    pub width: u32,
    pub pitch_or_linear_size: u32,
    pub depth: u32,
    pub mipmap_count: u32,
    pub reserved1: [u32; 11],
    pub format: DdsPixelformat,
    pub caps: u32,
    pub caps2: u32,
    pub caps3: u32,
    pub caps4: u32,
    pub reserved2: u32,
}

#[derive(BinRead)]
#[brw(little)]
pub struct DdsHeaderDXT10 {
    pub dxgi_format: u32,
    pub resource_dimension: u32,
    pub misc_flag: u32,
    pub array_size: u32,
    pub misc_flags2: u32,
}

#[derive(BinRead)]
#[brw(little)]
pub struct DdsPixelformat {
    pub size: u32,
    pub flags: u32,
    pub fourcc: u32,
    pub rgbbitcount: u32,
    pub rbitmask: u32,
    pub gbitmask: u32,
    pub bbitmask: u32,
    pub abitmask: u32,
}

impl<'a> DdsFile<'a> {
    pub fn parse(data: &'a [u8]) -> Result<Self> {
        let mut reader = std::io::Cursor::new(data);
        let magic = reader.read_u32::<LittleEndian>()?;
        if magic != DDS_MAGIC {
            return Err(anyhow::anyhow!("Invalid magic number"));
        }

        let header = DdsHeader::read(&mut reader)?;
        let dxt10 = if header.format.fourcc == 0x30315844 {
            Some(DdsHeaderDXT10::read(&mut reader)?)
        } else {
            None
        };

        let datapos = reader.position() as usize;

        Ok(Self {
            magic,
            header,
            dxt10,
            data: &reader.get_ref()[datapos..]
        })
    }

    pub fn texture_size(&self) -> TextureExtents {
        let width = self.header.width;
        let height = self.header.height;
        let depth = if self.header.flags & FLAG_DDSD_DEPTH != 0 { self.header.depth } else { 1 };
        let mip_count = if self.header.flags & FLAG_DDSD_MIPMAPCOUNT != 0 { self.header.mipmap_count } else { 1 };
        let mut array_layers = self.dxt10.as_ref().map(|x| x.array_size).unwrap_or(1);
        let is_cubemap = (self.header.caps2 & FLAG_DDSCAPS2_CUBEMAP != 0) && (self.header.caps2 & FLAGS_ALL_CUBEMAP_FACES != 0);

        if is_cubemap {
            array_layers /= 6;
        }

        TextureExtents {
            width, height, depth, array_layers, mip_count, is_cubemap
        }
    }

    pub fn format_dxgi(&self) -> DxgiFormat {
        if let Some(dxt10) = self.dxt10.as_ref() {
            return DxgiFormat::from_u32(dxt10.dxgi_format).unwrap();
        }
        unimplemented!("no dxt10 format");
    }

    pub fn format(&self) -> vk::Format {
        map_dxgi_format_to_vk(self.format_dxgi())
    }
}