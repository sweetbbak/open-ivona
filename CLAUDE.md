# Working in this repo (matching decomp of IVONA ARM32 .so files)

Read README.md first. The rules below are what keeps the project deterministic.

## Per-function loop

1. **Pick a function.** Use `./dc ls todo`, preferring leaves and functions whose callees are
   already named. Read it with `./dc asm FUNC`. You can also use Ghidra output. Treat
   `../eng.gbr.amy.1.6.101.3/*.so.c` (retdec) and `../open-ivona` as hints only, never as truth.
2. **Write C/C++.** Put it in the TU it plausibly came from, `src/<bin>/<module>.c|.cpp`.
   - Name the function exactly as in the tsv. Rename first with `./dc rename FUN_xxx good_name`.
     Use the real mangled name for C++ (keep it via the C++ signature) or `extern "C"`.
   - Put a short doc comment above it: address, mode, what it does.
3. **Diff.** Run `./dc diff FUNC -s src/<bin>/<file>`, then iterate until `MATCH`. Status, score
   and src are saved to `symbols/<bin>.funcs.tsv` automatically.
   `./dc tu src/<bin>/<file>` diffs every function a TU defines in one go. Prefer it for whole TUs.
4. **Check before committing.** Run `./dc check` (it must pass) and `./dc status`.

## Rules

- Never edit `target/`. `target.sha256` is checked on every load.
- Never relax the matcher to make a function pass. If the harness is wrong, fix it, then extend
  `tests/selftest.py` so the case stays covered and `./dc selftest` passes.
- Compiler flags live in `config/<bin>.yaml`.
  - Per-TU deviations go in `units:`, each with a comment saying why.
  - For experiments use `-X`, which never saves.
  - Run a different NDK with `DC_NDK=r9d`.
- **Static data referenced by code.** Define it in the TU, using `./dc dump` to generate the
  initializers. The matcher compares its bytes, strings and pointers.
  - A `HIDDEN` extern (include/common.h) only makes sense as a temporary stand-in. It can change
    register and operand order compared with a TU-local definition (seen on GetVoiceDataFunc).
- **`const` matters.** It decides whether data lands in `.data.rel.ro.local`, `.rodata` or `.data`,
  and the matcher checks this.
- **Library code is not hand-written.** Leave `lib` functions to `./dc libmatch` (run it for every
  binary with `-b`). It also:
  - folds splitter fragments back into one row when a merged run verifies (Thumb-1 `bl` far
    jumps, hand-written libgcc assembly);
  - pairs identical-code functions (`what()`, D0/D1 destructors) by object order next to verified
    neighbours;
  - resolves C1/C2 and D1/D2 aliases through `build/libobj/aliases.json`.
- **Vendored third-party code.** Statically linked libraries are built from pinned upstream
  releases (`external/fetch.sh`) by `./dc vendor build <lib>`, then verified by `./dc libmatch`.
  Recipes, flags and the evidence behind every configure option are in `config/vendor.yaml`.

  | Library | Engine block | Build |
  |---|---|---|
  | OpenSSL 1.0.2h libcrypto | 0xe1ffc–0x1af040 | ARM `-O0` + frame pointer, unsigned char, `no-err no-engine no-dsa no-dh no-ec*`, no `BN_LLONG` |
  | Expat 2.2.0 | ~0x19f000–0x1c9000 (~140 functions left) | `-O0`, asserts kept (no `-DNDEBUG`); the rest are a patched tree (asserts one line later: line 713 in `gather_time_entropy`) and probably another compiler (call results stored via `mov r3, r0; str r3`, unlike OpenSSL's `-O0`) |
  | libcurl 7.52.1 | ~0x1cb000–0x1f1000 (+ cold statics at 0x18924) | engine ARM `-O2`, unsigned char, symbol hiding, no SSL/IPv6/cookies/proxy/crypto-auth |
  | PCRE 8.37 | 0x72000–0xe0a00 | `-O0`, UTF + UCP, JIT (sljit), NO_RECURSE, NEWLINE=ANY; `real_pcre` is 8 bytes larger than upstream (local change) |
  | SILK SDK 1.0.8 (fixed point) | 0x202cac–0x20cb08 (complete) | engine ARM flags with `-O3` (the SDK Makefile's); vox codec 14 |

  Vendored archives are offered only to the binaries listed in their recipe.

  Not vendored yet (see `docs/notes/vox-codecs.md`):
  - **Tremor** (integer Vorbis decoder, used for vox codec 8/12) at ~0x1f9000–0x201500, built
    with the engine's ARM `-O2` flags. It's an older revision than xiph master:
    `codec_setup_info` is 0xcb0 bytes (master 0xca8) and `vorbis_comment_clear` has no NULL check.
    With master, 24 functions are byte-identical.
    Its MDCT code reaches 0x202c88, just before SILK.
  - Unoptimized code before Tremor (~0x1f1000) is still unidentified.
  - Static functions are per object: a table row named `name@member` (e.g.
    `constant_time_is_zero@rsa_pk1`) is that object's copy. The matcher accepts `name` or `name@…`
    and records the address as a placement, so one object's callers must agree on the copy.
  - libmatch ends with consistency passes. Data placement must agree per object. A lone row from
    another library inside a library's block is dropped (tiny `-O0` setters are byte-identical
    across libraries). Every library row is re-verified to a fixed point. So it never leaves a row
    `dc check` rejects.
  - Merged spans also start at rows with a prologue (`-O0` switch tables split functions). The
    reverse also works: a row that holds two functions (gold merges identical adjacent
    `.ARM.exidx` entries, hiding a tail-called neighbour) is offered up to an inner `push {..., lr}`,
    split tentatively so the tail call's target gets an auto name, and split for good if it
    verifies.
  - `./dc diff FUNC -s lib:<lib>:<member> [--sym NAME]` diffs a todo function against a library
    member. That's how every configure option above was found (struct offsets, missing calls).
- **Renaming.** `./dc rename` rewrites sources only for placeholder names (`FUN_`/`D_`); pass
  `--src` to rewrite a real name. Address comments above C++ functions resolve to the mangled
  symbol of the compiled object (`/* 0x4ecc: ... */` above `h(...)` names the row `h` or `_Z...`).
- **Thumb-1 output from GCC 4.8 `-Os`:**
  - `adds rX, rY, #0` is a move.
  - `bl __gnu_thumb1_case_*` is a switch.
  - GCC 4.8's reassoc pass merges `i < 0 || i > N` into one unsigned compare. Separate checks in
    the binary usually mean `if (i < 0) ...; size_t u = i; if (u >= N) ...`.

## Engine (libttsivonavoice) knowledge

- **Flags.** The engine core is ARM `-O2` without `-mtune=xscale`, plus `-fno-stack-protector`
  and `-fsigned-char`. See the comments in `config/libttsivonavoice.yaml`.
- **Embedded Flite.** The engine contains a modified CMU Flite 1.4: `cst_*`, `feat_*`, `val_*`.
  Run `external/fetch.sh` for the reference source, and put Flite TUs under
  `src/libttsivonavoice/flite/`. `include/flite/` holds Flite's headers, adapted to IVONA's
  changes:
  - `cst_safe_alloc(count, size)` replaces the size-only allocator.
  - `cst_error()` is `exit(-1)`.
  - `SWAPSHORT` works in unsigned int.
  - `typedef unsigned char cst_string;`. It shows in `cst_downcase`'s ctype lookups having no
    EOF/range checks.
  - Accessors (`val_int`, `val_car`, …) `cst_error()` silently. Only `val_<usertype>` accessors
    still print "VAL: tried to access %s in %d type val".
- **Flite workflow.**
  1. `./dc flite build`: compiles Flite 1.4 with the engine flags and the IVONA headers.
  2. `./dc flite locate --apply`: names located functions.
     See `docs/notes/flite-locate.md` for addresses and scores.
  3. `./dc flite import <dir/file.c>`: copies the Flite file into `src/libttsivonavoice/flite/`
     with address comments.
  4. `./dc tu` that file, then edit the functions that don't match yet (IVONA's changes).
  5. `./dc learn <file>`: names the FUN_/D_ callees and data (such as `cst_val_type_*`) that
     well-matching functions reference.

  Caution: the locator can swap twin functions whose code differs only in a referenced
  constant, such as `relation_val` and `item_val`. Check those against their neighbours and the
  type-id data names.
- **IVONA's HRG model.**
  - Relations are identified by an int id. `cst_relation_id_name(id)` (`0x4e510`, table
    `cst_relation_id_names`) gives the key used in feature sets. `utt_relation*` and `item_as`
    take ids.
  - Utterances have two engine pointers, from `new_utterance(a, b)`, and keep `ctx` at +0x18.
  - Every daughter item has `u` (parent) set.
  - `feat_present`, `feat_remove` and their item wrappers return a byte boolean. This shows as
    missing or extra tail calls.
  - `cst_dbgmsg` is compiled out but still evaluates its arguments.
- **IVONA's Flite struct changes.**
  - `cst_relation` is `{name (not copied), utterance, head, tail}`, with no features.
  - `CST_VAL_REGISTER_TYPE*` macros check the type inline and print "VAL: tried to access..."
    (see `include/flite/cst_val_defs.h`).
- **Function order.** Binary order follows source order within a TU. Functions missing from a
  Flite file were removed by `--gc-sections`.
- **libcurl is statically linked.** Seen as `curl_global_init` and friends near 0x1cb000.
- **Function names come from address comments.** A `/* 0xADDR ...` comment directly above a
  definition makes `./dc tu` rename the table entry at ADDR. That lets the table be rebuilt from
  source after a re-split.
- **Codegen hints.**
  - A single exit (`int ret = ...; ... return ret;`) shows as one `mov r0, rN; pop` block that the
    error paths branch to. Separate `return`s give per-path epilogues.
  - Argument evaluation order is visible. In `android_syslog`, `va_arg` inside the call is
    evaluated after `gettid()`.
  - A local array indexed by a variable, whose elements are also used by constant index, keeps
    register copies when scalar locals feed it (`r[MIN] = min = get(...)`).
  - If the binary lays the "else"/"different" case out inline, the source usually tests that
    case first, e.g. `if (type1 != type2) {...}` in `val_equal`.
  - A boolean materialized and then reused as NULL usually means the success path is inside the
    `if` and `return NULL` comes after it (seen in `cst_memdup`).
  - In an if/else chain, block layout follows source order (seen in `feat_print`).
  - A call result consumed after another load means it was a separate statement
    (seen in `feat_copy_into`).
