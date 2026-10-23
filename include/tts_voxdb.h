#ifndef TTS_VOXDB_H
#define TTS_VOXDB_H

/* vox unit databases (voxdb.c) */

/* a phone (32 bytes, section 7) */
typedef struct vox_phone {
    char name[32];
} vox_phone;

/* a decoded unit (0x9c bytes) */
typedef struct vox_unit {
    float dur;                          /* +0x00: seconds */
    const vox_phone *phones[5];         /* +0x04 */
    unsigned char f18, f19, f1a, f1b, f1c, f1d, f1e;
    unsigned char f1f[9];               /* +0x1f */
    unsigned char f28[5];               /* +0x28 */
    const vox_phone *ph30;              /* +0x30 */
    unsigned char f34, f35, f36;
    const vox_phone *ph38;              /* +0x38 */
    unsigned char f3c;
    const vox_phone *ph40;              /* +0x40 */
    unsigned char f44, f45, f46;
    const vox_phone *ph48;              /* +0x48 */
    unsigned char f4c;
    unsigned char f4d[6];               /* +0x4d */
    const vox_phone *ph54;              /* +0x54 */
    unsigned char f58, f59;
    unsigned char content38, content40, content48;      /* +0x5a */
    unsigned char unk_5d[3];
    unsigned char unk_60[0x28];         /* +0x60 */
    void *p88, *p8c, *p90, *p94;        /* +0x88 */
    unsigned int f98;                   /* +0x98 */
} vox_unit;

#endif
