pub mod dds_file;
pub mod textureinfo;
pub mod dxgi_format_map;

use clap::Parser;
use clap_derive::Parser;
use ktx2_rw::Ktx2Texture;
use vulkanite::vk;

#[derive(Parser)]
struct Args {
    /// Input DDS file
    input: String,

    /// Output KTX2 file
    output: String,
}

fn mipsize(width: u32, height: u32, depth: u32, format: vk::Format) -> usize {
    if format.is_compressed() {
        let blocksize = format.block_size() as usize;

        let w = (width.max(1) + 3) / 4;
        let h = (height.max(1) + 3) / 4;
        let d = depth.max(1);

        return (w * h * d) as usize * blocksize;
    } else {
        let bpp = format.block_size() as usize;
        return (width * height * depth) as usize * bpp;
    }
}

fn main() {
    let args = Args::parse();
    let input = std::fs::read(args.input).unwrap();
    let dds_file = dds_file::DdsFile::parse(&input).unwrap();
    let si = dds_file.texture_size();
    dds_file.texture_size().print_info();
    let format = dds_file.format();
    println!("format: {:?}", format);

    let vformat = ktx2_rw::VkFormat::B8G8R8A8Srgb;
    unsafe {
        let vformatptr = &vformat as *const _ as *mut u32;
        let formatptr = &format as *const _ as *const u32;
        formatptr.copy_to(vformatptr, 1);
    }
    let mut texture = Ktx2Texture::create(
        si.width, si.height, si.depth,
        si.array_layers, if si.is_cubemap { 6 } else { 1 },
        si.mip_count, vformat
    ).unwrap();

    let mut offset = 0_usize;
    for layer in 0..si.array_layers {
        let face_count = if si.is_cubemap { 6 } else { 1 };
        for face in 0..face_count {
            for level in 0..si.mip_count {
                let mipwidth = (si.width >> level).max(1);
                let mipheight = (si.height >> level).max(1);
                let mipdepth = (si.depth >> level).max(1);

                let mipsize = mipsize(mipwidth, mipheight, mipdepth, format);

                let mipdata = &dds_file.data[offset..offset + mipsize];
                texture.set_image_data(level, layer, face, mipdata).unwrap();
                offset += mipsize;
            }
        }
    }

    texture.set_metadata("KTXwriter", b"ddstoktx2").unwrap();

    texture.write_to_file(&args.output).unwrap();
    println!("wrote to {}", args.output);
}
