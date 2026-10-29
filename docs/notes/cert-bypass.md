# Certificate bypass

IVONA only loads voices after `tts_load_certificate*()` accepts a license certificate: a
`Key: value` text followed by a `-----BEGIN SIGNATURE-----` block. The signature is checked
against a 4096-bit RSA public key embedded in the engine (blob at `0x21ace8`), so a
certificate can't be forged. No signed certificate survives anywhere in this repo
(`src/certificate.txt` is empty, `IVONA 2 Voice/certificates/` is empty). The earlier
`open-ivona` bring-up patched out the check at `0x472e8` instead
(`open-ivona/re/oracle-frontend.md`).

We don't need licensing any more, so the decompiled engine bypasses it at build time:

- Define `IVONA_NO_CERT_CHECK` when building a runnable engine (`-DIVONA_NO_CERT_CHECK`).
- `cert_parse()` then ignores the certificate text and returns an unlimited certificate:
  never expires, no speed limit, no license (`license_id = -1`), voices `all` and
  `all:custom`. SSML is off by default (`IVONA_CERT_SSML`, see `ssml-deferred.md`), so all
  input takes the plain-text path. `cert_compatible()` accepts any pair and `cert_merge()` is a
  no-op.
- Its `key` is still the real one, the CRC-32 of the public key blob (the original sets it
  the same way after verification), because voice parameter values are obfuscated with it
  (`voice_param.c`).
- `tts_load_certificate_file()` still has to open a file, but any file works, including an
  empty one (`examples/example.c` wants `certificate.txt`).

The matching build never defines the flag. `cert_parse` (`0x42ab0`), `cert_compatible`,
`cert_merge` and the signature helpers (`0x46dec`, `0x4717c`, `0x47374`, `0x47468`) stay
`todo` and are not a priority.
