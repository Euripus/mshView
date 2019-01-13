#include "ImageData.h"
#include <cstring>
#include <fstream>
#include <iostream>

#pragma pack(push, 1)
struct BITMAPFILEHEADER
{
    uint16_t   bfType;               // bmp file signature
    uint32_t   bfSize;               // file size
    uint16_t   bfReserved1;
    uint16_t   bfReserved2;
    uint32_t   bfOffBits;
};

struct BITMAPINFO12                  // CORE version
{
    uint32_t  biSize;
    uint16_t  biWidth;
    uint16_t  biHeight;
    uint16_t  biPlanes;
    uint16_t  biBitCount;
};

struct BITMAPINFO                    // Standart version
{
    uint32_t  biSize;
    int32_t   biWidth;
    int32_t   biHeight;
    uint16_t  biPlanes;
    uint16_t  biBitCount;
    uint32_t  biCompression;
    uint32_t  biSizeImage;
    int32_t   biXPelsPerMeter;
    int32_t   biYPelsPerMeter;
    uint32_t  biClrUsed;
    uint32_t  biClrImportant;
};

struct TGAHEADER
{
    uint8_t  idlength;
    uint8_t  colourmaptype;
    uint8_t  datatypecode;
    uint16_t colourmaporigin;
    uint16_t colourmaplength;
    uint8_t  colourmapdepth;
    uint16_t x_origin;
    uint16_t y_origin;
    uint16_t width;
    uint16_t height;
    uint8_t  bitsperpixel;
    uint8_t  imagedescriptor;
};
#pragma pack(pop)

//==============================================================================
//         Read BMP section
//==============================================================================
bool ReadBMP(std::string fname, ImageData & id)
{
    bool res = false;
    bool compressed = false;
    bool flip = false;

    id.width = 0;
    id.height = 0;
    id.type = ImageData::PixelType::pt_none;
    if(id.data)
        id.data.reset(nullptr);

    std::ifstream ifs(fname, std::ios::in | std::ios::binary);
    if(!ifs.is_open())
    {
        std::cerr << "Error opening file: " << fname << std::endl;
        return res;
    }

    ifs.seekg(0, std::ios_base::end);
    auto file_length = ifs.tellg();
    ifs.seekg(0, std::ios_base::beg);
    if(file_length == 0)
        return res;

    // allocate memory:
    auto buffer = std::make_unique<char[]>(file_length);
    // read file as a block:
    ifs.read(buffer.get(), file_length);
    ifs.close();

    char * pPtr = buffer.get();
    BITMAPFILEHEADER * pHeader = reinterpret_cast<BITMAPFILEHEADER *>(pPtr);
    pPtr += sizeof(BITMAPFILEHEADER);
    if(pHeader->bfSize != file_length ||
            pHeader->bfType != 0x4D42)                   // little-endian
        return res;

    if(reinterpret_cast<uint32_t *>(pPtr)[0] == 12)
    {
        BITMAPINFO12 * pInfo = reinterpret_cast<BITMAPINFO12 *>(pPtr);
        pPtr += pInfo->biSize;

        if(pInfo->biBitCount != 24 && pInfo->biBitCount != 32)
            return res;

        if(pInfo->biBitCount == 24)
            id.type = ImageData::PixelType::pt_rgb;
        else
            id.type = ImageData::PixelType::pt_rgba;

        id.width = pInfo->biWidth;
        id.height = pInfo->biHeight;
    }
    else
    {
        BITMAPINFO * pInfo = reinterpret_cast<BITMAPINFO *>(pPtr);
        pPtr += pInfo->biSize;

        if(pInfo->biBitCount != 24 && pInfo->biBitCount != 32)
            return res;

        if(pInfo->biCompression != 3 && pInfo->biCompression != 6 &&
                pInfo->biCompression != 0)
        {
            return res;
        }
        
        if(pInfo->biCompression == 3 || pInfo->biCompression == 6)
        {
            compressed = true;
        }

        if(pInfo->biBitCount == 24)
            id.type = ImageData::PixelType::pt_rgb;
        else
            id.type = ImageData::PixelType::pt_rgba;

        id.width = pInfo->biWidth;
        
        if(pInfo->biHeight < 0)
            flip = true;
        id.height = std::abs(pInfo->biHeight);
    }
    
    // read data:
    pPtr = buffer.get() + pHeader->bfOffBits;
    uint32_t lineLength = 0;
    uint32_t bytes_per_pixel = (id.type == ImageData::PixelType::pt_rgb ? 3 : 4);
    auto image = std::make_unique<uint8_t[]>(id.width * id.height * bytes_per_pixel);
    uint8_t red, green, blue, alpha;
    uint32_t w_ind(0), h_ind(0);

    if(id.type == ImageData::PixelType::pt_rgb)
        lineLength = id.width * bytes_per_pixel + id.width % 4;
    else
        lineLength = id.width * bytes_per_pixel;

    for(uint32_t i = 0; i < id.height; ++i)
    {
        w_ind = 0;
        for(uint32_t j = 0; j < lineLength; j += bytes_per_pixel)
        {
            if(j > id.width * bytes_per_pixel)
                continue;

            if(compressed)
            {
                uint32_t count = 0;
                if(id.type == ImageData::PixelType::pt_rgba)
                {
                    alpha = pPtr[i * lineLength + j + count];
                    count++;
                }
                blue = pPtr[i * lineLength + j + count];
                count++;
                green = pPtr[i * lineLength + j + count];
                count++;
                red = pPtr[i * lineLength + j + count];
            }
            else
            {
                blue = pPtr[i * lineLength + j + 0];
                green = pPtr[i * lineLength + j + 1];
                red = pPtr[i * lineLength + j + 2];
                if(id.type == ImageData::PixelType::pt_rgba)              // !!!Not supported - the high byte in each DWORD is not used
                    alpha = pPtr[i * lineLength + j + 3];                 // https://msdn.microsoft.com/en-us/library/windows/desktop/dd183376(v=vs.85).aspx
            }


            image[h_ind * id.width * bytes_per_pixel + w_ind * bytes_per_pixel + 0] = red;
            image[h_ind * id.width * bytes_per_pixel + w_ind * bytes_per_pixel + 1] = green;
            image[h_ind * id.width * bytes_per_pixel + w_ind * bytes_per_pixel + 2] = blue;
            if(id.type == ImageData::PixelType::pt_rgba)
                image[h_ind * id.width * bytes_per_pixel + w_ind * bytes_per_pixel + 3] = alpha;
            w_ind++;
        }
        h_ind++;
    }
    
    //flip image if necessary
    if(flip)
    {
        auto temp_buf = std::make_unique<uint8_t[]>(id.width * id.height * bytes_per_pixel);
        
        for(uint32_t i = 0; i < id.height; i++)
        {
            std::memcpy(temp_buf.get() + i * id.width  * bytes_per_pixel,
                        image.get() + (id.height - 1 - i) * id.width  * bytes_per_pixel,
                        id.width  * bytes_per_pixel);
        }
        
        image = std::move(temp_buf);
    }

    id.data = std::move(image);
    res = true;

    return res;
}

//==============================================================================
//         TGA section
//==============================================================================
bool WriteTGA(std::string fname, const ImageData & id)
{
    bool res = false;
    TGAHEADER tga;
    std::memset(&tga, 0, sizeof(tga));
    uint32_t bytes_per_pixel = (id.type == ImageData::PixelType::pt_rgb ? 3 : 4);

    std::ofstream ofs(fname, std::ios::out | std::ios::binary);
    if(!ofs.is_open())
    {
        std::cerr << "Error opening file: " << fname << std::endl;
        return res;
    }

    tga.datatypecode = 2;
    tga.width = id.width;
    tga.height = id.height;
    tga.bitsperpixel = bytes_per_pixel * 8;
    if(id.type == ImageData::PixelType::pt_rgb)
        tga.imagedescriptor = 0x10;
    else
        tga.imagedescriptor = 0x18;

    ofs.write(reinterpret_cast<char *>(&tga), sizeof(tga));

    uint8_t * data_ptr = id.data.get();
    uint8_t red, green, blue, alpha;
    for(uint32_t i = 0; i < id.width * id.height * bytes_per_pixel; i += bytes_per_pixel)
    {
        red = data_ptr[i + 0];
        green = data_ptr[i + 1];
        blue = data_ptr[i + 2];
        if(bytes_per_pixel == 4)
            alpha = data_ptr[i + 3];

        ofs.write(reinterpret_cast<char *>(&blue), 1);
        ofs.write(reinterpret_cast<char *>(&green), 1);
        ofs.write(reinterpret_cast<char *>(&red), 1);
        if(id.type == ImageData::PixelType::pt_rgba)
            ofs.write(reinterpret_cast<char *>(&alpha), 1);
    }

    ofs.close();
    res = true;

    return res;
}

bool ReadUncompressedTGA(ImageData & id, char * data);
bool ReadCompressedTGA(ImageData & id, char * data);

bool ReadTGA(std::string fname, ImageData & id)
{
    std::ifstream ifs(fname, std::ios::in | std::ios::binary);
    if(!ifs.is_open())
    {
        std::cerr << "Error opening file: " << fname << std::endl;
        return false;
    }
    
    ifs.seekg(0, std::ios_base::end);
    auto file_length = ifs.tellg();
    ifs.seekg(0, std::ios_base::beg);
    if(file_length == 0)
        return false;
        
    // allocate memory:
    auto buffer = std::make_unique<char[]>(file_length);
    // read file as a block:
    ifs.read(buffer.get(), file_length);
    ifs.close();
    
    char * pPtr = buffer.get();
    TGAHEADER * pHeader = reinterpret_cast<TGAHEADER *>(pPtr);
    pPtr += sizeof(TGAHEADER);
    
    if(pHeader->datatypecode == 2)
    {
        return ReadUncompressedTGA(id, buffer.get());
    }
    else if(pHeader->datatypecode == 10)
    {
        return ReadCompressedTGA(id, buffer.get());
    }
    
    return false;
}

bool ReadUncompressedTGA(ImageData & id, char * data)
{
    uint8_t * pPtr = reinterpret_cast<uint8_t *>(data);
    TGAHEADER * pHeader = reinterpret_cast<TGAHEADER *>(pPtr);
    pPtr += sizeof(TGAHEADER);
    
    if((pHeader->width <= 0) || (pHeader->height <= 0) 
        || ((pHeader->bitsperpixel != 24) && (pHeader->bitsperpixel !=32)))         // Make sure all information is valid
    {
        return false;
    }
    
    id.width = pHeader->width;
    id.height = pHeader->height;
    id.type = pHeader->bitsperpixel == 24 ? ImageData::PixelType::pt_rgb 
                                          : ImageData::PixelType::pt_rgba;
    bool flip_horizontal =  (pHeader->imagedescriptor & 0x10);
    bool flip_vertical =    (pHeader->imagedescriptor & 0x20);
    
    uint32_t bytes_per_pixel = pHeader->bitsperpixel/8;
    uint32_t image_size = id.width * id.height * bytes_per_pixel;
    
    auto img = std::make_unique<uint8_t[]>(image_size);
    
    uint8_t red, green, blue, alpha;
    for(uint32_t i = 0; i < id.width * id.height; ++i)
    {
        red = pPtr[i * bytes_per_pixel + 2];
        green = pPtr[i * bytes_per_pixel + 1];
        blue = pPtr[i * bytes_per_pixel + 0];
        if(id.type == ImageData::PixelType::pt_rgba)
            alpha = pPtr[i * bytes_per_pixel + 3];
            
        img[i * bytes_per_pixel + 0] = red;
        img[i * bytes_per_pixel + 1] = green;
        img[i * bytes_per_pixel + 2] = blue;
        if(id.type == ImageData::PixelType::pt_rgba)
            img[i * bytes_per_pixel + 3] = alpha;
    }
    
    if(flip_vertical)
    {
        auto flipped_img = std::make_unique<uint8_t[]>(image_size);
        
        for(uint32_t i = 0; i < id.height; i++)
        {
            std::memcpy(flipped_img.get() + i * id.width  * bytes_per_pixel,
                        img.get() + (id.height - 1 - i) * id.width  * bytes_per_pixel,
                        id.width  * bytes_per_pixel);
        }
        
        img = std::move(flipped_img);
    }
    
    if(flip_horizontal)
    {
        auto flipped_img = std::make_unique<uint8_t[]>(image_size);
        
        for(uint32_t i = 0; i < id.height; i++)
        {
            for(uint32_t j = 0; j < id.width; j++)
            {
                flipped_img[id.width  * bytes_per_pixel * i + j * bytes_per_pixel + 0] = 
                    img[id.width  * bytes_per_pixel * i + (id.width - j - 1) * bytes_per_pixel + 0];
                flipped_img[id.width  * bytes_per_pixel * i + j * bytes_per_pixel + 1] = 
                    img[id.width  * bytes_per_pixel * i + (id.width - j - 1) * bytes_per_pixel + 1];
                flipped_img[id.width  * bytes_per_pixel * i + j * bytes_per_pixel + 2] = 
                    img[id.width  * bytes_per_pixel * i + (id.width - j - 1) * bytes_per_pixel + 2];
                if(id.type == ImageData::PixelType::pt_rgba)
                    flipped_img[id.width  * bytes_per_pixel * i + j * bytes_per_pixel + 3] = 
                        img[id.width  * bytes_per_pixel * i + (id.width - j - 1) * bytes_per_pixel + 3];
            }
        }
        
        img = std::move(flipped_img);
    }
    
    id.data = std::move(img);
    return true;
}

bool ReadCompressedTGA(ImageData & id, char * data)
{
    uint8_t * pPtr = reinterpret_cast<uint8_t *>(data);
    TGAHEADER * pHeader = reinterpret_cast<TGAHEADER *>(pPtr);
    pPtr += sizeof(TGAHEADER);
    
    if((pHeader->width <= 0) || (pHeader->height <= 0) 
        || ((pHeader->bitsperpixel != 24) && (pHeader->bitsperpixel !=32)))         // Make sure all information is valid
    {
        return false;
    }
    
    id.width = pHeader->width;
    id.height = pHeader->height;
    id.type = pHeader->bitsperpixel == 24 ? ImageData::PixelType::pt_rgb 
                                          : ImageData::PixelType::pt_rgba;
    bool flip_horizontal =  (pHeader->imagedescriptor & 0x10);
    bool flip_vertical =    (pHeader->imagedescriptor & 0x20);
    
    uint32_t bytes_per_pixel = pHeader->bitsperpixel/8;
    uint32_t image_size = id.width * id.height * bytes_per_pixel;
    
    auto     img = std::make_unique<uint8_t[]>(image_size);
    uint32_t pixelcount = id.height * id.width;
    uint32_t currentpixel   = 0;
    uint32_t currentbyte    = 0;
    
    do
    {
        uint8_t chunk = pPtr[0];
        pPtr++;
        
        if(chunk & 128)
        {
            chunk -= 127;
            for(short counter = 0; counter < chunk; counter++)
            {                                                               
                img[currentbyte + 0] = pPtr[2];
                img[currentbyte + 1] = pPtr[1];
                img[currentbyte + 2] = pPtr[0];
                if(id.type == ImageData::PixelType::pt_rgba)                                    
                    img[currentbyte + 3] = pPtr[3];


                currentbyte += bytes_per_pixel; 
                currentpixel++; 

                if(currentpixel > pixelcount)                                           // Make sure we havent written too many pixels
                {
                    return false;
                }
            }
            pPtr += bytes_per_pixel;
        }
        else
        {
            chunk++;
            for(short counter = 0; counter < chunk; counter++)
            {           
                img[currentbyte + 0] = pPtr[2];
                img[currentbyte + 1] = pPtr[1];
                img[currentbyte + 2] = pPtr[0];
                if(id.type == ImageData::PixelType::pt_rgba)
                    img[currentbyte + 3] = pPtr[3];
                    
                currentbyte += bytes_per_pixel;
                currentpixel++; 
                pPtr += bytes_per_pixel;
                
                if(currentpixel > pixelcount)                                           // Make sure we havent read too many pixels
                {
                    return false;                                                       
                }
            }
        }
    }while(currentpixel < pixelcount);
    
    if(flip_vertical)
    {
        auto flipped_img = std::make_unique<uint8_t[]>(image_size);
        
        for(uint32_t i = 0; i < id.height; i++)
        {
            std::memcpy(flipped_img.get() + i * id.width  * bytes_per_pixel,
                        img.get() + (id.height - 1 - i) * id.width  * bytes_per_pixel,
                        id.width  * bytes_per_pixel);
        }
        
        img = std::move(flipped_img);
    }
    
    if(flip_horizontal)
    {
        auto flipped_img = std::make_unique<uint8_t[]>(image_size);
        
        for(uint32_t i = 0; i < id.height; i++)
        {
            for(uint32_t j = 0; j < id.width; j++)
            {
                flipped_img[id.width  * bytes_per_pixel * i + j * bytes_per_pixel + 0] = 
                    img[id.width  * bytes_per_pixel * i + (id.width - j - 1) * bytes_per_pixel + 0];
                flipped_img[id.width  * bytes_per_pixel * i + j * bytes_per_pixel + 1] = 
                    img[id.width  * bytes_per_pixel * i + (id.width - j - 1) * bytes_per_pixel + 1];
                flipped_img[id.width  * bytes_per_pixel * i + j * bytes_per_pixel + 2] = 
                    img[id.width  * bytes_per_pixel * i + (id.width - j - 1) * bytes_per_pixel + 2];
                if(id.type == ImageData::PixelType::pt_rgba)
                    flipped_img[id.width  * bytes_per_pixel * i + j * bytes_per_pixel + 3] = 
                        img[id.width  * bytes_per_pixel * i + (id.width - j - 1) * bytes_per_pixel + 3];
            }
        }
        
        img = std::move(flipped_img);
    }
    
    id.data = std::move(img);
    return true;
}
