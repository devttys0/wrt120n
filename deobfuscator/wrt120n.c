/*
 * Deobfuscator for WRT120N firmware images.
 *
 * Craig Heffner
 * http://www.devttys0.com
 * 2014-01-15
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "crc.h"

#define OBFUSCATION_MAGIC       "\x04\x01\x09\x20"
#define OBFUSCATION_MAGIC_SIZE  4

#define MAX_IMAGE_SIZE          0x1B0000

#define BLOCK_SIZE              32
#define BLOCK1_OFFSET           4
#define BLOCK2_OFFSET           0x68
#define MIN_FILE_SIZE           (OBFUSCATION_MAGIC_SIZE + BLOCK2_OFFSET + BLOCK_SIZE)

#define CRC32_SIZE              4
#define CRC32_OFFSET            0x14

void block_swap(char *data)
{
    char block1_copy[BLOCK_SIZE] = { 0 };
        
    /* Swap blocks 1 and 2 */
    printf("Doing block swap...\n");
    memcpy(block1_copy, data+BLOCK1_OFFSET, BLOCK_SIZE);
    memcpy(data+BLOCK1_OFFSET, data+BLOCK2_OFFSET, BLOCK_SIZE);
    memcpy(data+BLOCK2_OFFSET, block1_copy, BLOCK_SIZE);
}

void nibble_swap(char *data)
{
    int i = 0;

    /* Nibble-swap the first 32 bytes of data */
    printf("Doing nibble-swap...\n");
    for(i=BLOCK1_OFFSET; i<(BLOCK1_OFFSET+BLOCK_SIZE); i++)
    {
        data[i] = ((data[i] & 0x0F) << 4) + ((data[i] & 0xF0) >> 4);
    }
}

void byte_swap(char *data)
{
    int i = 0, j = 0, k = 0;

    /* Byte swap 16 two-byte pairs */
    printf("Doing byte-swap...\n");
    for(i=0; i<(BLOCK_SIZE/2); i++)
    {
        j = data[BLOCK1_OFFSET+(i*2)];
        k = data[BLOCK1_OFFSET+(i*2)+1];

        data[BLOCK1_OFFSET+(i*2)] = k;
        data[BLOCK1_OFFSET+(i*2)+1] = j;
    }
}

int crc(char *data, size_t size)
{
    uint32_t new_crc = 0;

    size -= (size & 0x3FF);
    memset(data+(size-CRC32_OFFSET), 0xFF, CRC32_SIZE);
    
    new_crc = crc32(data, size) ^ 0xFFFFFFFF;
    memcpy(data+(size-CRC32_OFFSET), (void *) &new_crc, CRC32_SIZE);

    return new_crc;
}

void decode(char *data, size_t size)
{
    block_swap(data);
    nibble_swap(data);
    byte_swap(data);
}

void encode(char *data, size_t size)
{
    byte_swap(data);
    nibble_swap(data);
    block_swap(data);

    printf("New CRC: 0x%X\n", crc(data, size));
}


int main(int argc, char *argv[])
{
    FILE *fp = NULL;
    size_t size = 0;
    int retval = EXIT_FAILURE;
    void (*action)(char *, size_t) = NULL;
    struct stat file_info = { 0 };
    char *target_file = NULL, *out_file = NULL, *data = NULL;

    if(argc < 3)
    {
        fprintf(stderr, "Usage: %s [-e] <firmware image file> <output file>\n", argv[0]);
        exit(retval);
    }

    out_file = argv[argc-1];
    target_file = argv[argc-2];

    if(argc == 4)
    {
        if(strcmp(argv[1], "-e") == 0)
        {
            action = encode;
        }
    }

    if(!action)
    {
        action = decode;
    }

    if(stat(target_file, (struct stat *) &file_info) == 0)
    {
        size = file_info.st_size;

        if(size >= MIN_FILE_SIZE)
        {
            fp = fopen(target_file, "rb");
            if(fp)
            {
                data = malloc(size);
                if(data)
                {
                    memset(data, 0, size);
                    if(fread(data, size, 1, fp) != 1)
                    {
                        perror("fread");
                    }
                }
                else
                {
                    perror("malloc");
                }

                fclose(fp);
            }
            else
            {
                perror("fopen");
            }
        }
        else
        {
            fprintf(stderr, "ERROR: File too small!\n");
        }
    }
    else
    {
        perror("stat");
    }

    if(data && size)
    {
        if(memcmp(data, OBFUSCATION_MAGIC, OBFUSCATION_MAGIC_SIZE) != 0)
        {
            printf("WARNING: Image does not start with the expected magic bytes!\nTrying anyway...\n");
        }

        if(size > MAX_IMAGE_SIZE)
        {
            printf("WARNING: Image exceeds maximum image size (%d)\n", MAX_IMAGE_SIZE);
        }

        /* Encode / decode the data */
        action(data, size);

        /* That's it! Write it to disk. */
        printf("Saving data to %s...\n", out_file);
        fp = fopen(out_file, "wb");
        if(fp)
        {
            if(fwrite(data, size, 1, fp) == 1)
            {
                printf("Done!\n");
                retval = EXIT_SUCCESS;
            }
            else
            {
                perror("fopen");
            }

            fclose(fp);
        }
        else
        {
            perror("fopen");
        }
    }

    if(data) free(data);
    return retval;
}
