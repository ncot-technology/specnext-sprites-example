#include <arch/zxn.h>
#include <z80.h>
#include <input.h>
#include <stdlib.h>
#include <arch/zxn/esxdos.h>
#include <stdbool.h>
#include <intrinsic.h>
#include <errno.h>

/* Sprites over layer 2 screen over ULA screen (default). */
#define LAYER_PRIORITIES_S_L_U 0x0

/* Layer 2 screen over sprites over ULA screen. */
#define LAYER_PRIORITIES_L_S_U 0x1

/* Sprites over ULA screen over layer 2 screen. */
#define LAYER_PRIORITIES_S_U_L 0x2

/* Layer 2 screen over ULA screen over sprites. */
#define LAYER_PRIORITIES_L_U_S 0x3

/* ULA screen over sprites over layer 2 screen. */
#define LAYER_PRIORITIES_U_S_L 0x4

/* ULA screen over layer 2 screen over sprites. */
#define LAYER_PRIORITIES_U_L_S 0x5

/* Max sprites per scanline limit reached. */
#define MAX_SPRITES_PER_SCANLINE_MASK 0x02

/* A collision between two or more sprites. */
#define SPRITE_COLLISION_MASK 0x01

/* Mirror the sprite x-wise. */
#define MIRROR_X_MASK 0x08

/* Mirror the sprite y-wise. */
#define MIRROR_Y_MASK 0x04

/* Rotate the sprite 90 degrees. */
#define ROTATE_MASK 0x02

#define SPRITE_PATTERN_MASK     0x3F
#define SPRITE_ATTRIB_MASK      0x7F
#define X_LSB_MASK           0x00FF
#define X_MSB_MASK           0x0100
#define X_MSB_SHIFT          8
#define PALETTE_OFFSET_SHIFT 4
#define SPRITE_VISIBLE_MASK  0x80
#define SPRITE_ENABLE_ATTRIB_4 0x40
#define SPRITE_REL_UNIFIED_MASK     0x20
#define X_EXT(x) (((uint16_t) (x)) + 32)
#define X_LSB(x_ext) (uint8_t) ((x_ext) & X_LSB_MASK)
#define X_MSB(x_ext) (uint8_t) (((x_ext) & X_MSB_MASK) >> X_MSB_SHIFT)
#define Y_EXT(y) ((y) + 32)
#define LAYER_PRIORITIES_MASK  0x07
#define LAYER_PRIORITIES_SHIFT 2

#define WAIT_FOR_SCANLINE(line)         while (ZXN_READ_REG(REG_ACTIVE_VIDEO_LINE_L) == line); \
                                        while (ZXN_READ_REG(REG_ACTIVE_VIDEO_LINE_L) != line)

void set_sprite_layers_system(bool sprites_visible,
                              bool sprites_on_border,
                              uint8_t layer_priorities,
                              bool lores_mode)
{
    uint8_t value = (layer_priorities & LAYER_PRIORITIES_MASK) << LAYER_PRIORITIES_SHIFT;

    if (sprites_visible)
    {
        value = value | RSLS_SPRITES_VISIBLE;
    }

    if (sprites_on_border)
    {
        value = value | RSLS_SPRITES_OVER_BORDER;
    }

    if (lores_mode)
    {
        value = value | RSLS_ENABLE_LORES;
    }

    IO_NEXTREG_REG = REG_SPRITE_LAYER_SYSTEM;
    IO_NEXTREG_DAT = value;
}

void set_sprite_attributes_ext(uint8_t sprite_pattern_slot,
                               uint8_t x,
                               uint8_t y,
                               uint8_t palette_offset,
                               uint8_t sprite_flags,
                               bool visible)
{
    uint8_t pattern_slot = sprite_pattern_slot & SPRITE_PATTERN_MASK;

    if (visible)
    {
        pattern_slot = pattern_slot | SPRITE_VISIBLE_MASK;
    }

    IO_SPRITE_ATTRIBUTE = X_LSB(X_EXT(x));
    IO_SPRITE_ATTRIBUTE = Y_EXT(y);
    IO_SPRITE_ATTRIBUTE = (palette_offset << PALETTE_OFFSET_SHIFT) | X_MSB(X_EXT(x)) | sprite_flags;
    IO_SPRITE_ATTRIBUTE = pattern_slot;
}

void set_sprite_attributes(uint8_t sprite_pattern_slot,
                           uint16_t x,
                           uint8_t y,
                           uint8_t palette_offset,
                           uint8_t sprite_flags,
                           bool visible)
{
    uint8_t pattern_slot = sprite_pattern_slot & SPRITE_PATTERN_MASK;

    if (visible)
    {
        pattern_slot = pattern_slot | SPRITE_VISIBLE_MASK;
    }

    IO_SPRITE_ATTRIBUTE = X_LSB(x);
    IO_SPRITE_ATTRIBUTE = y;
    IO_SPRITE_ATTRIBUTE = (palette_offset << PALETTE_OFFSET_SHIFT) | X_MSB(x) | sprite_flags;
    IO_SPRITE_ATTRIBUTE = pattern_slot;
}

void set_sprite_pattern(const void *sprite_pattern)
{
    intrinsic_outi((void *) sprite_pattern, __IO_SPRITE_PATTERN, 256);
}

void set_sprite_pattern_slot(uint8_t sprite_slot)
{
    IO_SPRITE_SLOT = sprite_slot & SPRITE_PATTERN_MASK;
}

void set_sprite_attrib_slot(uint8_t sprite_slot)
{
    IO_SPRITE_SLOT = sprite_slot & SPRITE_ATTRIB_MASK;
}

void set_sprite_palette(const uint16_t *colors, uint16_t length, uint8_t palette_index)
{
    uint8_t *color_bytes = (uint8_t *) colors;
    uint16_t i;

    if ((colors == NULL) || (length == 0))
    {
        return;
    }

    if (palette_index + length > 256)
    {
        length = 256 - palette_index;
    }

    IO_NEXTREG_REG = REG_PALETTE_INDEX;
    IO_NEXTREG_DAT = palette_index;

    IO_NEXTREG_REG = REG_PALETTE_VALUE_16;
    for (i = 0; i < (length << 1); i++)
    {
        IO_NEXTREG_DAT = color_bytes[i];
    }
}

void load_sprite_patterns(const char *filename,
                          const void *sprite_pattern_buf,
                          uint8_t num_sprite_patterns,
                          uint8_t start_sprite_pattern_slot)
{
    uint8_t filehandle;

    if ((filename == NULL) || (sprite_pattern_buf == NULL) ||
        (num_sprite_patterns == 0) || (start_sprite_pattern_slot > 63))
    {
        return;
    }

    if (start_sprite_pattern_slot + num_sprite_patterns > 64)
    {
        num_sprite_patterns = 64 - start_sprite_pattern_slot;
    }

    errno = 0;
    filehandle = esxdos_f_open(filename, ESXDOS_MODE_R | ESXDOS_MODE_OE);
    if (errno)
    {
        return;
    }

    set_sprite_pattern_slot(start_sprite_pattern_slot);

    while (num_sprite_patterns--)
    {
        esxdos_f_read(filehandle, (void *) sprite_pattern_buf, 256);
        if (errno)
        {
            break;
        }
        set_sprite_pattern(sprite_pattern_buf);
    }

    esxdos_f_close(filehandle);
}

void load_sprite_palette(const char *filename, const void *sprite_palette_buf)
{
    uint8_t filehandle;

    if ((filename == NULL) || (sprite_palette_buf == NULL))
    {
        return;
    }

    errno = 0;
    filehandle = esxdos_f_open(filename, ESXDOS_MODE_R | ESXDOS_MODE_OE);
    if (errno)
    {
        return;
    }

    esxdos_f_read(filehandle, (void *) sprite_palette_buf, 256);
    if (errno)
    {
        goto end;
    }
    set_sprite_palette((uint16_t *) sprite_palette_buf, 128, 0);

    esxdos_f_read(filehandle, (void *) sprite_palette_buf, 256);
    if (errno)
    {
        goto end;
    }
    set_sprite_palette((uint16_t *) sprite_palette_buf, 128, 128);

end:
    esxdos_f_close(filehandle);
}

int main(void)
{
    zx_border(INK_BLUE);
    uint8_t sprBuf[256];
    load_sprite_patterns("all.spr", sprBuf, 37, 0);
    set_sprite_pattern_slot(0);
    load_sprite_palette("all.nxp", sprBuf);
    set_sprite_layers_system(true, true, LAYER_PRIORITIES_S_L_U, false);
    set_sprite_attrib_slot(0);

    // Endless loop
    int timer = 0;
    while(1) {
        set_sprite_attrib_slot(0);
        set_sprite_attributes(0, 128, 32, 0, 0, 1);
        set_sprite_attributes(1, 128, 64, 0, 0, 1);
        WAIT_FOR_SCANLINE(192);
        timer++;          
    }

    return 0;
}

  /* C source end */
