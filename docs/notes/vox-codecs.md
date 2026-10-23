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
  | 14 | `FUN_0006b260`, `FUN_0006b320`, `FUN_0006b354` | very likely **SILK SDK 1.0.8**: `FUN_00202cac` stores a decoder size 0x3570 and returns 0 (`SKP_Silk_SDK_Get_Decoder_Size`); `FUN_00202cc4` checks `fs_kHz * 1000` against the requested sample rate (`SKP_Silk_SDK_Decode`); the `"1.0.8"` getter at 0x20324c (`SKP_Silk_SDK_get_version`); error codes -11/-12 |
  | other | failure | |

## Which voices use which codec

| file | header name | rate | +0x28 |
|---|---|---|---|
| Android voice package OBB `main.199.com.ivona.tts.voicebeta.eng.gbr.amy.obb` | vox_en_gb_amy | 22050 | **14** |
| Kindle `vox_en_au_nicole22i` (ivona 1.6.30) | vox_en_au_nicole | 22050 | **14** |
| IVONA 2 (Windows) `vox_en_gb_amy22v` | vox_en_gb_amy | 22050 | 12 |
| IVONA 2 (Windows) `vox_fr_celine22v` | vox_fr_celine | 22050 | 12 |

So the Android voices are decoded with the SILK-like codec. Vorbis (Tremor) is linked and
reachable for codec 8/12 vox files, such as those of the Windows IVONA 2 product.

## Still to verify

- Build the SILK SDK 1.0.8 decoder with the engine flags and match the block at ~0x201500–0x20c000.
- The `-O0` code at ~0x1f1000 (before Tremor) is still unidentified.
