# Vox database audio codecs (engine)

How the engine picks the codec for a voice's unit audio, traced on 2026-10-22.

## Codec selection

- **`FUN_0005f370` (voice load).** Reads the voice features `voxdbname` and `voxdb_format`
  (`"22v"`). It memory-maps the vox file (`cst_mmap_file`) and hands the mapping to
  `FUN_0005f1e4`.
- **`FUN_0005e938` (codec factory).** Copies the 0x68-byte header and dispatches on the magic:
  - `CDXV` (`0x56584443`): seven fixed (offset, size) section pairs at +0x30 … +0x64, which become
    section types 1–7.
  - `DDXV` (`0x56584444`): +0x30 must be 1, +0x34 is a count of 12-byte section entries at +0x68.
- **Codec choice.** The codec comes from header **+0x28**: a jump table on `value - 2` stores three
  function pointers (taken through the GOT) at codec object +0x48/+0x4c/+0x50 or +0x54.

  | +0x28 | functions | codec |
  |---|---|---|
  | 2 | `FUN_0006b1a8` | ? |
  | 8 | `FUN_0006b6dc`, `FUN_0006b82c`, `FUN_0006b868` | Vorbis (Tremor): `FUN_0006b868` decodes length-prefixed packets with `vorbis_synthesis` / `_pcmout` / `_read`, output `>> 9` clipped to int16 |
  | 12 | `FUN_0006bb58` (init), `FUN_0006bebc`, `FUN_0006bef8` (decode) | Vorbis (Tremor): init calls `vorbis_info_init`, `vorbis_comment_init` (byte-identical to Tremor), header parsing, `vorbis_synthesis_init`; decode as above |
  | 14 | `FUN_0006b260` (init: `SKP_Silk_SDK_Get_Decoder_Size`), `FUN_0006b320` (free), `FUN_0006b354` (decode: `SKP_Silk_SDK_InitDecoder`, `SKP_Silk_SDK_Decode`) | **SILK SDK 1.0.8**, fixed-point: verified byte for byte, see below |
  | other | failure | |

## Which voices use which codec

| file | header name | rate | +0x28 |
|---|---|---|---|
| Android voice package OBB `main.199.com.ivona.tts.voicebeta.eng.gbr.amy.obb` | vox_en_gb_amy | 22050 | **14** |
| Kindle `vox_en_au_nicole22i` (ivona 1.6.30) | vox_en_au_nicole | 22050 | **14** |
| IVONA 2 (Windows) `vox_en_gb_amy22v` | vox_en_gb_amy | 22050 | 12 |
| IVONA 2 (Windows) `vox_fr_celine22v` | vox_fr_celine | 22050 | 12 |

So the Android voices are decoded with SILK. Vorbis (Tremor) is linked and
reachable for codec 8/12 vox files, such as those of the Windows IVONA 2 product.

## SILK verification

- **Source.** SILK SDK 1.0.8 fixed-point (`SILK_SDK_SRC_FIX_v1.0.8`, pinned in `external/fetch.sh`
  from a copy in fingi/csipsimple by commit and git tree hash). `./dc vendor build silk`.
- **Flags.** The engine's ARM flags with **-O3**, as in the SDK's own Makefile (-O2 inlines
  differently in `SKP_Silk_lin2log`, `SKP_Silk_biquad`, `SKP_Silk_CNG_Reset`).
- **Result.** All 70 functions of the block 0x202cac–0x20cb08 are verified (`lib:silk:...`),
  including encoder-side helpers that came along with their objects (range encoder, gain
  quantiser, `SKP_Silk_encode_signs`).
- **Entry points.** `SKP_Silk_SDK_Get_Decoder_Size` 0x202cac, `SKP_Silk_SDK_Decode` 0x202cc4,
  `SKP_Silk_SDK_get_version` 0x20324c, `SKP_Silk_decode_frame` 0x20325c,
  `SKP_Silk_init_decoder` 0x209a88.

## Still open

- **Codec-14 wrappers.** `FUN_0006b260`/`FUN_0006b320`/`FUN_0006b354` (IVONA code) and how units
  are laid out in the vox sections for them.
- **Tremor's tail.** Code just before SILK (0x201594–0x202c88, MDCT butterflies) belongs to it.
  The `-O0` code at ~0x1f1000 is still unidentified.
