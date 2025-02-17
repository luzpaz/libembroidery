/*
 *  Thanks to Jason Weiler for describing the binary formats of the HUS and
 *  VIP formats at:
 *
 *  http://www.jasonweiler.com/HUSandVIPFileFormatInfo.html
 *
 *  Further thanks to github user tatarize for solving the mystery of the
 *  compression in:
 *
 *  https://github.com/EmbroidePy/pyembroidery
 *
 *  with a description of that work here:
 *
 *  https://stackoverflow.com/questions/7852670/greenleaf-archive-library
 *
 *  This is based on their work.
 */

/* This is a work in progress.
 * ---------------------------
 */

#include "embroidery.h"

#include <stdlib.h>
#include <string.h>

typedef struct Huffman {
    int default_value;
    int *lengths;
    int nlengths;
    int *table;
    int table_width;
    int ntable;
} huffman;

void huffman_init(huffman *h, int lengths, int value);
void huffman_build_table(huffman *h);
void huffman_table_lookup(huffman *h, int byte_lookup, int *value, int *lengths);
void huffman_free(huffman *h);

typedef struct Compress {
    int bit_position;
    char *input_data;
    int input_length;
    int bits_total;
    int block_elements;
    huffman *character_length_huffman;
    huffman *character_huffman;
    huffman *distance_huffman;
} compress;

int compress_get_bits(compress *c, int length);
int compress_pop(compress *c, int bit_count);
int compress_read_variable_length(compress *c);
int compress_load_character_length_huffman(compress *c);
void compress_load_character_huffman(compress *c);
void compress_load_distance_huffman(compress *c);
void compress_load_block(compress *c);
int compress_get_token(compress *c);
int compress_get_position(compress *c);

/* This avoids the now unnecessary compression by placing a
 * minimal header of 6 bytes and using only literals in the
 * huffman compressed part (see the sources above).
 */
int hus_compress(char *data, int length, char *output, int *output_length)
{
    unsigned char *a = (unsigned char*)output;
    a[0] = length%256;
    a[1] = (length/256)%256;
    a[2] = 0x02;
    a[3] = 0xA0;
    a[4] = 0x01;
    a[5] = 0xFE;
    memcpy(output+6, data, length);
    *output_length = length+6;
    return 0;
}

/* These next 4 functions represent the Huffman class in tartarize's code.
 */
void huffman_init(huffman *h, int lengths, int value)
{
    /* these mallocs are guessing for now */
    h->default_value = value;
    h->lengths = malloc(1000);
    h->nlengths = 0;
    h->table = malloc(1000);
    h->ntable = 0;
    h->table_width = 0;
}

void huffman_build_table(huffman *h)
{
    int bit_length, i, max_length, size;
    max_length = 0;
    size = 1 << h->table_width;
    for (i = 0; i < h->table_width; i++) {
        if (h->lengths[i] > max_length) {
            max_length = h->lengths[i];
        }
    }
    for (bit_length=1; bit_length<=h->table_width; bit_length++) {
        int j;
        size /= 2;
        for (j=0; j < h->nlengths; j++) {
            if (h->lengths[j] == bit_length) {
                int k;
                for (k=0; k<size; k++) {
                    h->table[h->ntable+k] = j;
                    h->ntable++;
                }
            }
        }
    }
}

void huffman_lookup(huffman *h, int* out, int byte_lookup)
{
    if (h->table == 0) {
        out[0] = h->default_value;
        out[1] = 0;
        return;
    }
    out[0] = h->table[byte_lookup >> (16-h->table_width)];
    out[1] = h->lengths[out[0]];
}

void huffman_free(huffman *h)
{
    free(h->table);
    free(h->lengths);
}

/* These functions represent the EmbCompress class. */
void compress_init()
{

}

int compress_get_bits(compress *c, int length)
{
    int i, end_pos_in_bits, start_pos_in_bytes,
        end_pos_in_bytes, value, mask_sample_bits,
        unused_bits, original;

    end_pos_in_bits = c->bit_position + length - 1;
    start_pos_in_bytes = c->bit_position / 8;
    end_pos_in_bytes = end_pos_in_bits / 8;
    value = 0;

    for (i=start_pos_in_bytes; i < end_pos_in_bytes+1; i++) {
        value <<= 8;
        if (i > c->input_length) {
            break;
        }
        value |= (c->input_data[i]) & 0xFF;
    }

    unused_bits = (7 - end_pos_in_bits) % 8;
    mask_sample_bits = (1<<length) - 1;
    original = (value >> unused_bits) & mask_sample_bits;
    return original;
}

int compress_pop(compress *c, int bit_count)
{
    int value = compress_get_bits(c, bit_count);
    c->bit_position += bit_count;
    return value;
}

int compress_peek(compress *c, int bit_count)
{
    return compress_get_bits(c, bit_count);
}

int compress_read_variable_length(compress *c)
{
    int q, m, s;
    m = compress_pop(c, 3);
    if (m!=7) {
        return m;
    }
    for (q=0; q<13; q++) {
        s = compress_pop(c, 1);
        if (s) {
            m++;
        }
        else {
            break;
        }
    }
    return m;
}

int compress_load_character_length_huffman(compress *c)
{
    int count;
    huffman h;
    count = compress_pop(c, 5);
    if (count == 0) {
        /*v = compress_pop(c, 5);*/
        /* huffman = huffman_init(huffman, v); ? */
    }
    else {
        int i;
        for (i = 0; i < count; i++) {
            h.lengths[i] = 0;
        }
        for (i = 0; i < count; i++) {
            if (i==3) {
                i += compress_pop(c, 2);
            }
            h.lengths[i] = compress_read_variable_length(c);
        }
        h.nlengths = count;
        h.default_value = 8;
        huffman_build_table(&h);
    }
    c->character_length_huffman = &h;
    return 1;
}

void compress_load_character_huffman(compress *c)
{
    int count;
    huffman h;
    count = compress_pop(c, 9);
    if (count == 0) {
        /*
        v = compress_pop(c, 9);
        huffman = huffman(v);
        */
    }
    else {
        int i;
        for (i = 0; i < count; i++) {
            h.lengths[i] = 0;
        }
        i = 0;
        while (i < count) {
            int h[2];
            huffman_lookup(c->character_length_huffman, h, compress_peek(c, 16));
            c->bit_position += h[1];
            if (h[0]==0) {
                i += h[0];
            }
            else if (h[0]==1) {
                i += 3 + compress_pop(c, 4);
            }
            else if (h[0]==2) {
                i += 20 + compress_pop(c, 9);
            }
            else {
                c->character_huffman->lengths[i] = h[0] - 2;
                i++;
            }
        }
        huffman_build_table(c->character_huffman);
    }
}

void compress_load_distance_huffman(compress *c)
{
    int count;
    huffman h;
    count = compress_pop(c, 5);
    if (count == 0) {
        /*
        v = compress_pop(c, 5);
        c->distance_huffman = Huffman(v);
        */
    }
    else {
        int i;
        for (i = 0; i < count; i++) {
            h.lengths[i] = 0;
        }
        for (i = 0; i < count; i++) {
            h.lengths[i] = compress_read_variable_length(c);
        }
        huffman_build_table(&h);
    }
    c->distance_huffman = &h;
}
    
void compress_load_block(compress *c)
{
    c->block_elements = compress_pop(c, 16);
    compress_load_character_length_huffman(c);
    compress_load_character_huffman(c);
    compress_load_distance_huffman(c);
}

int compress_get_token(compress *c)
{
    int h[2];
    if (c->block_elements <= 0) {
        compress_load_block(c);
    }
    c->block_elements--;
    huffman_lookup(c->character_huffman, h, compress_peek(c, 16));
    c->bit_position += h[1];
    return h[0];
}

int compress_get_position(compress *c)
{
    int h[2];
    int v;
    huffman_lookup(c->distance_huffman, h, compress_peek(c, 16));
    c->bit_position += h[1];
    if (h[0] == 0) {
        return 0;
    }
    v = h[0] - 1;
    v = (1<<v) + compress_pop(c, v);
    return v;
}

int hus_decompress(char *data, int length, char *output, int *output_length)
{
    int character, i, j;
    compress *c = (compress*)malloc(sizeof(compress));
    c->bit_position = 0;
    c->input_data = data;
    c->input_length = length;
    c->bits_total = length*8;
    i = 0;
    while (c->bits_total > c->bit_position && i < *output_length) {
        /* process token */
        character = 0; /* fix this */
        if (character < 0x100) {
            output[i] = (char)character;
            i++;
        }
        else if (character == 510) {
            break;
        }
        else {
            length = character - 253;
            /* not sure about i here */
            c->bit_position = i - compress_get_position(c) - 1;
            for (j=c->bit_position; j < c->bit_position+length; j++) {
                output[i] = output[j];
                i++;
            }
        }
    }
    free(c);
    return 0;
}

