/*
 * write-exif.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <libexif/exif-data.h>

#include "deftypes.h"
#include "write_exif.h"

/* raw JFIF header data */
static const unsigned char jfif_header[] = { 0xff, 0xd8, 0xff, 0xe0 };

/* raw EXIF header data */
static const unsigned char exif_header[] = { /* 0xff, 0xd8,*/ 0xff, 0xe1 };

/* byte order to use in the EXIF block */
#define FILE_BYTE_ORDER EXIF_BYTE_ORDER_INTEL

/* special header required for EXIF_TAG_USER_COMMENT */
#define ASCII_COMMENT "ASCII\0\0\0"
#define ASCII_HDR_LEN (sizeof(ASCII_COMMENT) - 1)

#if 0
/* Get an existing tag, or create one if it doesn't exist */
static ExifEntry *init_tag(ExifData *exif, ExifIfd ifd, ExifTag tag)
{
    ExifEntry *entry;
    /* Return an existing tag if one exists */
    if (!((entry = exif_content_get_entry (exif->ifd[ifd], tag))))
    {
        /* Allocate a new entry */
        entry = exif_entry_new ();
        assert(entry != NULL);  /* catch an out of memory condition */
        entry->tag = tag;       /* tag must be set before calling exif_content_add_entry */

        /* Attach the ExifEntry to an IFD */
        exif_content_add_entry (exif->ifd[ifd], entry);

        /* Allocate memory for the entry and fill with default data */
        exif_entry_initialize (entry, tag);

        /* Ownership of the ExifEntry has now been passed to the IFD.
         * One must be very careful in accessing a structure after
         * unref'ing it; in this case, we know "entry" won't be freed
         * because the reference count was bumped when it was added to
         * the IFD.
         */
        exif_entry_unref(entry);
    }
    
    return entry;
}
#endif

static ExifEntry *create_tag(ExifData *exif, ExifIfd ifd, ExifTag tag, size_t len)
{
    void *buf;
    ExifEntry *entry;

    /* Create a memory allocator to manage this ExifEntry */
    ExifMem *mem = exif_mem_new_default();
    if(mem == NULL)
    {
        log_err("exif mem new failed.");
        return NULL;
    }
    
    /* Create a new ExifEntry using our allocator */
    entry = exif_entry_new_mem (mem);
    if(entry == NULL)
    {
        exif_mem_unref(mem);
        log_err("exif entry new failed.");
        return NULL;
    }

    /* Allocate memory to use for holding the tag data */
    buf = exif_mem_alloc(mem, len);
    if(buf == NULL)
    {
        exif_mem_unref(mem);
        exif_entry_unref(entry);
        log_err("exif mem alloc failed.");
        return NULL;
    }

    /* Fill in the entry */
    entry->data = buf;
    entry->size = len;
    entry->tag = tag;
    entry->components = len;
    entry->format = EXIF_FORMAT_UNDEFINED;

    /* Attach the ExifEntry to an IFD */
    exif_content_add_entry (exif->ifd[ifd], entry);

    /* The ExifMem and ExifEntry are now owned elsewhere */
    exif_mem_unref(mem);
    exif_entry_unref(entry);

    return entry;
}

static int add_exif_to_jpeg_file(const char *path, unsigned char *exif_data, unsigned int exif_data_len)
{
    FILE *f;
    unsigned char *jpeg_data;
    unsigned int jpeg_len;
    unsigned int jfif_app0_size;
    int ret;

    if ((path == NULL) || (exif_data == NULL) || (exif_data_len == 0))
    {
        log_eno("pls check input para: %p-%p-%d", path, exif_data, exif_data_len);
        return 0;    
    }

    f = fopen(path, "rb+");
    if(!f)
    {
        log_eno("pic path '%s' invalid.", path);
        return 0;
    }

    fseek(f, 0, SEEK_END);
    jpeg_len = ftell(f);

    jpeg_data = malloc(jpeg_len);
    if(!jpeg_data)
    {
        log_eno("malloc size %d failed.", jpeg_len);
        fclose(f);
        return 0;
    }
    
    fseek(f, 0, SEEK_SET);
    if(fread(jpeg_data, 1, jpeg_len, f) != jpeg_len)
    {
        log_eno("Could not read '%s'.", path);
        goto ERR;
    }

    ret = memcmp(jpeg_data, jfif_header, sizeof(jfif_header));
    if(ret != 0)
    {
        log_err("jfif header[%02x %02x] err.", jpeg_data[2], jpeg_data[3]);
        goto ERR;
    }

    /* 不包括jfif_header */
    jfif_app0_size = (jpeg_data[4] << 8) | jpeg_data[5];
    //log_info("jfif app0 size %d\n", jfif_app0_size);
    //log_hex(jpeg_data, (4 + jfif_app0_size));
    
    /* 跳过jfif信息，后写入exif信息 */
    fseek(f, sizeof(jfif_header) + jfif_app0_size, SEEK_SET);

    /* Write EXIF header */
    if (fwrite(exif_header, sizeof(exif_header), 1, f) != 1)
    {
        log_err("Error writing to file %s\n", path);
        goto ERR;
    }

    /* Write EXIF block length in big-endian order */
    if (fputc((exif_data_len + 2) >> 8, f) < 0)
    {
        log_err("Error writing to file %s\n", path);
        goto ERR;
    }
    if (fputc((exif_data_len + 2) & 0xff, f) < 0)
    {
        log_err("Error writing to file %s\n", path);
        goto ERR;
    }

    /* Write EXIF data block */
    if (fwrite(exif_data, exif_data_len, 1, f) != 1)
    {
        log_err("Error writing to file %s\n", path);
        goto ERR;
    }

    /* Write JPEG image data, skipping the non-EXIF header */
    if (fwrite(jpeg_data + sizeof(jfif_header) + jfif_app0_size, jpeg_len - sizeof(jfif_header) - jfif_app0_size, 1, f) != 1)
    {
        log_err("Error writing to file %s\n", path);
        goto ERR;
    }
    
    free(jpeg_data);
    fclose(f);
    return 1;

ERR:
    free(jpeg_data);
    fclose(f);
    return 0;    
}

/* 需要调用者释放exif内存 */
int format_exif_comment(char *text, int text_len, unsigned char **data)
{
    unsigned char *exif_data;
    unsigned int exif_data_len;
    ExifEntry *entry;
    ExifData *exif;

    exif = exif_data_new();
    if (!exif)
    {
        log_err("Out of memory\n");
        return 0;
    }

#if 0
    /* Set the image options */
    exif_data_set_option(exif, EXIF_DATA_OPTION_FOLLOW_SPECIFICATION);
    exif_data_set_data_type(exif, EXIF_DATA_TYPE_COMPRESSED);
    exif_data_set_byte_order(exif, FILE_BYTE_ORDER);

    /* Create the mandatory EXIF fields with default data */
    exif_data_fix(exif);

    /* All these tags are created with default values by exif_data_fix() */
    /* Change the data to the correct values for this image. */
    entry = init_tag(exif, EXIF_IFD_EXIF, EXIF_TAG_PIXEL_X_DIMENSION);
    exif_set_long(entry->data, FILE_BYTE_ORDER, image_jpg_x);

    entry = init_tag(exif, EXIF_IFD_EXIF, EXIF_TAG_PIXEL_Y_DIMENSION);
    exif_set_long(entry->data, FILE_BYTE_ORDER, image_jpg_y);

    entry = init_tag(exif, EXIF_IFD_EXIF, EXIF_TAG_COLOR_SPACE);
    exif_set_short(entry->data, FILE_BYTE_ORDER, 1);
#endif
    
    entry = create_tag(exif, EXIF_IFD_EXIF, EXIF_TAG_USER_COMMENT, ASCII_HDR_LEN + text_len);
    memcpy(entry->data, ASCII_COMMENT, ASCII_HDR_LEN);
    memcpy(entry->data + ASCII_HDR_LEN, text, text_len);

#if 0
    /* Create a EXIF_TAG_SUBJECT_AREA tag */
    entry = create_tag(exif, EXIF_IFD_EXIF, EXIF_TAG_SUBJECT_AREA,
               4 * exif_format_get_size(EXIF_FORMAT_SHORT));
    entry->format = EXIF_FORMAT_SHORT;
    entry->components = 4;
    exif_set_short(entry->data, FILE_BYTE_ORDER, image_jpg_x / 2);
    exif_set_short(entry->data + 2, FILE_BYTE_ORDER, image_jpg_y / 2);
    exif_set_short(entry->data + 4, FILE_BYTE_ORDER, image_jpg_x);
    exif_set_short(entry->data + 6, FILE_BYTE_ORDER, image_jpg_y);
#endif

    /* Get a pointer to the EXIF data block we just created */
    exif_data_save_data(exif, &exif_data, &exif_data_len);
    exif_data_unref(exif);
    if(exif_data == NULL)
    {
        log_err("exif_data \n");
        return 0;
    }

    *data = exif_data;
    return exif_data_len;
}


int write_exif_comment(char *file, char *text, int text_len)
{
    unsigned char *exif_data;
    unsigned int exif_data_len;
    int ret;

    exif_data_len = format_exif_comment(text, text_len, &exif_data);
    if(exif_data_len == 0)
    {
        log_err("format exif failed.");
        return 0;
    }

    ret = add_exif_to_jpeg_file(file, exif_data, exif_data_len);
    free(exif_data);
    if(ret == 0)
    {
        log_err("add exif failed.");
        return 0;
    }

    return 1;
}

