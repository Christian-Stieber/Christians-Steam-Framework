// https://protobuf.dev/programming-guides/encoding/

/************************************************************************/
/*
 * g++ -ggdb -std=c++20 Decode.cpp
 */

/************************************************************************/
/*
 * This reads something that looks like a hexadecimal dump (must have
 * 0x prefixes) of protobuf data, and decodes it.
 *
 * Example: echo "0x8 0x96 0x01" | ./a.out
 */

/************************************************************************/

#include <cassert>
#include <span>
#include <iostream>
#include <vector>
#include <sstream>

/************************************************************************/

static std::span<const uint8_t> bytes;

class EndOfBytes { };

/************************************************************************/

static uint8_t getByte()
{
    if (bytes.empty())
    {
        throw EndOfBytes();
    }
    auto c=bytes.front();
    bytes=bytes.last(bytes.size()-1);
    return c;
}

/************************************************************************/

static uint64_t getVARINT()
{
    uint64_t result=0;
    int offset=0;
    while (offset<10)
    {
        auto c=getByte();
        result|=(c&0x7f)<<(7*offset);
        if (!(c&0x80))
        {
            return result;
        }
        offset++;
    }
    std::cerr << "Bad VARINT" << std::endl;
    assert(false);
}

/************************************************************************/

static uint64_t getFixed(int size)
{
    uint64_t result=0;
    for (int offset=0; offset<size; offset++)
    {
        auto c=getByte();
        result|=c<<(8*offset);
    }
    return result;
}

/************************************************************************/

static void decodeItem()
{
    uint64_t tag=getVARINT();
    uint8_t wireType=tag&0x7;
    uint64_t fieldNumber=tag>>3;
    std::cout << "fieldNumber " << fieldNumber << "; type ";
    switch(wireType)
    {
    case 0:	// VARINT
        {
            auto value=getVARINT();
            std::cout << "VARINT; value " << value << " (0x" << std::hex << value << std::dec << ")" << std::endl;
        }
        break;

    case 1: // I64
        {
            auto value=getFixed(8);
            std::cout << "I64; value " << value << " (0x" << std::hex << value << std::dec << ")" << std::endl;
        }
        break;

    case 2:	// LEN
        {
            auto len=getVARINT();
            std::cout << "LEN " << len;
            for (uint64_t i=0; i<len; i++)
            {
                if ((i&0xf)==0)
                {
                    std::cout << std::endl;
                    std::cout << "    ";
                }
                auto c=getByte();
                std::cout << " 0x" << std::hex << (unsigned int)c << std::dec;
            }
            std::cout << std::endl;
        }
        break;

    case 5:	// I32
        {
            auto value=getFixed(4);
            std::cout << "I32; value " << value << " (0x" << std::hex << value << std::dec << ")" << std::endl;
        }
        break;

    default:
        std::cerr << "Unsupported wire type: " << wireType << std::endl;
        assert(false);
    }
}

/************************************************************************/

static std::vector<uint8_t> readData()
{
    std::stringstream file;
    file << std::cin.rdbuf();

    std::vector<uint8_t> data;
    
    auto string=file.view();
    while (!string.empty())
    {
        auto c=string.front();
        if (c==' ' || c=='\n' || c=='\r' || c=='\t' || c==',')
        {
            string.remove_prefix(1);
        }
        else
        {
            if (string.starts_with("0x"))
            {
                int counter=0;
                uint8_t byte=0;
                string.remove_prefix(2);
                while (!string.empty())
                {
                    c=string.front();
                    if (c>='0' && c<='9')
                    {
                        byte=(byte<<4)+(c-'0');
                    }
                    else if (c>='a' && c<='f')
                    {
                        byte=(byte<<4)+(c-'a'+10);
                    }
                    else if (c>='A' && c<='F')
                    {
                        byte=(byte<<4)+(c-'A'+10);
                    }
                    else
                    {
                        break;
                    }
                    counter++;
                    string.remove_prefix(1);
                }
                if (counter==1 || counter==2)
                {
                    data.push_back(byte);
                }
            }
        }
    }
    return data;
}

/************************************************************************/

int main()
{
    auto data=readData();
    bytes=data;
    while (!bytes.empty())
    {
        decodeItem();
    }
    return 0;
}
