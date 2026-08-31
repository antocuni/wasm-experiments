# 15 - WASI sockets

Goal: write a small C program that uses TCP sockets, compile it to WASI with
`zig cc`, run it on stock `wasmtime` -- and from that, design a lightweight,
pip-installable toolchain for SPy (a static-Python compiler that emits C and
compiles it to wasm, and wants sockets on both native and wasi).

This single README contains: the TL;DR + quick start, the **requirements** that
drive the design, the background mental model (core module vs component, the roles
of the tools, wasi-libc), the hard constraints that rule out simpler ideas, the
**recommended** toolchain, and the alternatives we considered and discarded.

---

## TL;DR

- Real BSD sockets (`socket`/`bind`/`listen`/`connect`/`accept`) exist only on the
  **`wasm32-wasip2`** target of wasi-libc and run on **wasmtime 48** with
  `-S inherit-network -S tcp`. The final artifact is a WASI Preview 2 **component**.
- You **can** keep `zig cc` as the compiler *and* `zig wasm-ld` as the linker, but
  the wasip2 socket implementation (libc + headers) must come from wasi-sdk, and a
  **componentization step** is unavoidable at build time.
- **Recommended toolchain:** stock `ziglang` (compiler+linker) + a small extracted
  wasip2 sysroot (headers + `libc.a` + crt + builtins, ~1.4 MB gzipped,
  host-independent) + `wasm-tools` (componentizer, single binary, on PyPI). No full
  wasi-sdk install, no `wasm-component-ld`.
- SPy should emit **standard BSD socket C**; the same source compiles for native
  (host libc) and wasi (extracted wasip2 libc).

## Requirements (why the tradeoffs are what they are)

Every choice below is a consequence of these goals. They sometimes conflict; the
recommended toolchain is the best compromise, and the "Alternatives" and "Open
concern" sections record what each requirement forced us to give up.

1. **Real sockets, on stock `wasmtime`.** TCP client *and* server, run on an
   unmodified upstream `wasmtime` (no custom host embedding). => forces the
   wasip2 / component path (only wasip2 has `connect`; wasmtime only satisfies
   `wasi:sockets/*` for components). This is why a componentization step is
   unavoidable and why the output is a component, not a plain core module.

2. **Keep the "compiler installs via pip" experience.** SPy already gets its
   compiler from the `ziglang` PyPI package (~10 MB, `zig cc` + `zig wasm-ld`,
   native + wasm). Adding wasi support must not require a separate, manual,
   heavyweight install. => rules out "just depend on the full wasi-sdk" (~200 MB,
   separate download); favors `wasm-tools` (also on PyPI) over `wasm-component-ld`
   (per-host binary, not on PyPI) as the componentizer.

3. **Small, ideally host-independent footprint.** What we ship on top of `ziglang`
   should be a few MB of data, not a second SDK. => ship only the extracted wasip2
   sysroot (headers + `libc.a` + crt + builtins, ~1.4 MB gzipped, one bundle for
   all platforms); do not bundle a per-host componentizer if `wasm-tools` (pip) can
   do the job.

4. **One socket implementation across native and wasi, in standard C.** SPy's
   emitted code should be portable BSD-socket C that compiles unchanged for native
   (host libc) and wasi (wasip2 libc) -- no bespoke shim ABI, no per-target socket
   API in the emitted code. => rules out the "socket-shims `.a`" approach (custom
   `shim_*` ABI); favors compiling ordinary `<sys/socket.h>` code with the real
   wasip2 headers.

5. **Extensible to other components later (bonus).** The same mechanism should
   scale to components with no native equivalent, e.g. **WASI HTTP**. => the build
   flow (compile C -> link core module -> componentize) must be generic; component-
   specific glue (via `wit-bindgen`) is added as extra objects, not baked into the
   toolchain.

6. **Don't silently regress code quality (constraint, not a goal we achieved).**
   The wasi build should not produce meaningfully worse code than a normal hosted
   target. This one is **partially violated** by the recommended recipe: the
   `--target=wasm32-freestanding` workaround (needed to stop zig hijacking headers)
   disables libc-builtin/idiom recognition. See "Open concern" below -- it is
   acceptable for the socket demo but must be evaluated before defaulting the whole
   SPy-to-wasi toolchain to it.

## Quick start

```sh
python3 -m venv venv-zig016 && venv-zig016/bin/pip install ziglang==0.16
./make-zig.sh   # compile with zig cc, link+componentize -> server/client.wasm
./run.sh        # start the server, run the client, print the echo round-trip
```

Expected output:

```
connected to 127.0.0.1:8080
sent: hello from a WASI socket
echo reply: hello from a WASI socket
```

## Files

- `server.c` - TCP echo server: `socket`/`bind`/`listen`/`accept`/`recv`/`send`.
- `client.c` - TCP client: `socket`/`connect`/`send`/`recv`.
- `make-zig.sh` - build the recommended way: `zig cc` + `zig wasm-ld` +
  `wasm-tools` with an external wasip2 sysroot (the SPy path).
- `make-wasi-sdk.sh` - reference build with plain wasi-sdk `wasm32-wasip2-clang`.
- `run.sh` - start the server, run the client, print the round-trip.
- `clean.sh` - remove build artifacts.
- `config.sh` - shared paths, sourced by the other scripts.
- `create-bundle.sh` - assembles the minimal C-only sysroot bundle from a wasi-sdk.
- `zig-wasip1-accept.c` - historical: the only socket-ish thing plain `zig cc`
  (no external sysroot) can compile, the wasip1 accept-subset.

---

# Background: the mental model

## Core module vs component

- **Core module** = "plain old wasm" (what `zig cc`/`clang`/`wasm-ld` emit).
  Header `00 61 73 6d 01 00 00 00` (layer 0). Imports/exports are flat
  `i32/i64/f32/f64` functions over one linear memory. wasip1 (files, stdio,
  clocks) is expressed entirely at this level, so a wasip1 module is directly
  runnable and linkable like any C lib.
- **Component** = an outer wrapper (header `...0d 00 01 00`, layer 1) around one
  or more core modules. It adds typed interfaces (strings, records, **resources**
  = opaque host-owned handles) via the Canonical ABI. wasmtime picks the code path
  by reading these 8 header bytes.

## Who does what

- **Compilers** (`zig cc`, clang, `wasm-ld`): always emit **core modules**. Even
  targeting wasip2, the output is a core module whose imports are named
  `wasi:sockets/tcp@0.2.12` etc. and which carries `component-type` custom sections
  describing how to wrap it later.
- **`wasm-tools component new`** / **`wasm-component-ld`**: the componentization
  step (both use the `wit-component` library). They read the `component-type`
  sections and wrap the core module into a **component**. `wasm-component-ld` also
  does the linking (it drives a `wasm-ld`); `wasm-tools` only componentizes an
  already-linked core module. This step is **required** for wasip2 (sockets),
  **skipped** for pure wasip1.
- **wasmtime**: runs core modules via the classic flat-import path (binds
  `wasi_snapshot_preview1`); runs components via the component path (resource
  tables + Canonical ABI), which is the **only** path that can bind
  `wasi:sockets/*`.

## Role of wasi-libc

wasi-libc provides the C-visible `socket()`/`connect()`/`bind()`/`listen()`/
`accept()`/`send()`/`recv()`. Crucially:

- On **wasip1** it provides only `accept`/`recv`/`send`/`shutdown` on a
  host-provided fd. No `socket`/`connect`/`bind`/`listen`. And stock wasmtime >=47
  removed the host path that handed over listener fds, so wasip1 sockets are dead.
- On **wasip2** it provides the full BSD API, implemented as ~thousands of lines of
  glue (`descriptor_table.c`, `sockets_utils.c`, `connect.c`, ...) that turn the
  async, stream-based, resource-based `wasi:sockets` interface into blocking BSD
  calls. These bottom out into `wasi:sockets/tcp` component imports.

`wasi-sdk` itself is three separable things: (1) a **compiler** (clang/LLVM +
`wasm-ld`) -- which `ziglang` already provides via `zig cc` / `zig wasm-ld`; (2) a
**sysroot** (wasi-libc headers + prebuilt `libc.a`, `crt1*.o`,
`libclang_rt.builtins.a`) -- plain host-independent data; (3) **`wasm-component-ld`**
-- a componentizer binary. Only (2) is truly needed on top of `ziglang`; (3) can be
replaced by `wasm-tools`.

## Hard constraints (why there is no "plain wasip1-style" socket lib)

1. **TCP sockets on stock wasmtime require the wasip2 / component path.** Outbound
   `connect()` exists ONLY in wasip2; there is no preview1 syscall for it.
2. **The component step cannot be baked into a plain `.a` at build time.** It is a
   whole-module transform that rewrites the binary from layer 0 to layer 1, done
   after linking. A core module importing `wasi:sockets/*` will not instantiate on
   wasmtime ("unknown import").
3. **No p2->p1 reverse adapter exists** (structurally impossible: resources have no
   preview1 representation). The only p1<->p2 adapter goes p1->p2 and is a
   guest-side module composed in at componentization time.
4. The only way to get a *plain core module* with sockets is a **custom wasmtime
   embedding** that implements socket host functions itself -- i.e. ship your own
   runtime, not stock `wasmtime`. Rejected: we want stock `wasmtime`.

Conclusion: sockets => wasip2 => the output is a **component**, and a
componentization step at build time is unavoidable. It can only be hidden behind a
wrapper command, not eliminated.

## Why plain `zig cc` alone can't do it

1. **wasi-libc split** (see above): no `socket`/`connect`/`bind`/`listen` on wasip1,
   and the wasip1 preopened-listener path was removed from modern wasmtime
   (`-S preview2=n` is a hard error since wasmtime 47; `-S tcplisten` no longer
   works).
2. **zig hijacks headers.** For any `wasm*-wasi*` triple, `zig cc` force-injects its
   own bundled `wasm-wasi-musl` headers ahead of `-nostdinc`/`-isystem`. zig
   0.13/0.15 headers don't declare sockets at all; zig 0.16 headers declare them
   under `-D__wasilibc_use_wasip2` but zig ships **no wasip2 socket libc**, so
   linking fails with `undefined symbol: socket`. The fix is the freestanding trick
   below.

---

# Recommended toolchain: ziglang + wasi-libc + wasm-tools

The best tradeoff for a pip-installable SPy toolchain (validated end-to-end: full
TCP echo round-trip on wasmtime 48). Keep `zig cc` as compiler AND linker; get the
wasip2 libc + headers from wasi-sdk (extracted into a small package); componentize
with `wasm-tools`. **No `wasm-component-ld`, no full wasi-sdk install.**

```sh
# headers + libc.a + crt1 + builtins extracted from wasi-sdk (host-independent)
SYSINC=<pkg>/include/wasm32-wasip2
SYSLIB=<pkg>/lib/wasm32-wasip2
BUILTINS=<pkg>/lib/wasm32-wasip2/libclang_rt.builtins.a

# 1. compile ORDINARY C (real <sys/socket.h>, socket(), connect(), ...) with zig cc.
#    --target=wasm32-freestanding stops zig injecting its own (socket-less) wasi
#    headers so our -isystem wins; re-assert __wasi__; rename main so wasi crt1's
#    __main_void -> __main_argc_argv resolves (else it traps at startup).
zig cc --target=wasm32-freestanding -D__wasi__ -Dmain=__main_argc_argv \
       -nostdinc -isystem "$SYSINC" -O2 -c -o app.o app.c

# 2. link to a core module with zig's own wasm-ld
zig wasm-ld -L"$SYSLIB" "$SYSLIB/crt1-command.o" app.o -lc "$BUILTINS" -o app_core.wasm

# 3. componentize with wasm-tools (no adapter needed; libc carries component-type)
wasm-tools component new app_core.wasm -o app.wasm

# 4. run
wasmtime run -S inherit-network -S tcp app.wasm 127.0.0.1 8080
```

(`make-zig.sh` in this directory implements this recipe; historically it used
`wasm-component-ld` for step 2+3, which also works -- see alternatives below.)

## Why this is best for SPy

- **Keep pip `ziglang` (~10 MB).** zig stays compiler + linker; the native target is
  untouched.
- **Sockets, one implementation.** SPy emits *standard BSD socket C*. The SAME
  source compiles for native (host libc) and wasi (wasi-sdk libc). No custom shim
  ABI, no `AF_INET` constant fiddling -- the app gets the correct wasip2 constants
  from wasi-sdk headers at compile time.
- **Arbitrary components later (WASI HTTP).** Same pipeline. For pure-WIT interfaces
  with no libc wrapper, add `wit-bindgen c`-generated glue as extra `.o`s;
  compile/link/componentize is unchanged.
- **Avoid the big wasi-sdk (~200 MB).** Ship only the extracted sysroot +
  `wasm-tools`.

## Load-bearing details / gotchas

- The three compile flags (`--target=wasm32-freestanding`, `-D__wasi__`,
  `-Dmain=__main_argc_argv`) MUST travel together. Plain `--target=wasm32-wasi`
  fails: zig force-injects its own headers even under `-nostdinc -isystem`, and they
  don't declare `socket()`. Without `-Dmain=__main_argc_argv`, `main` stays plain
  `main`, crt1's `__main_void` can't find `__main_argc_argv`, and the module traps
  at startup with `undefined_weak:main`.
- **No duplicate libc.** SPy's `.o` references only libc *symbols* (`printf`,
  `malloc`, ...), not a specific libc; compiling with `-c` and doing the final link
  ourselves against the wasip2 `libc.a` means exactly one libc ends up in the binary
  (verified: `malloc`/`free`/`printf` appear once; no `wasi_snapshot_preview1`
  leakage). Never let zig link its own wasip1 libc into a wasip2 module.
- **Version pinning.** The `component-type` metadata embedded in `libc.a` is
  versioned; pin the wasi-sdk sysroot to a compatible `wasm-tools` + `wasmtime`.
  Validated: wasi-sdk-34 + wasm-tools 1.254 + wasmtime 48. (A mismatched
  `wasm-tools component new` can panic with `wit-component ... assertion failed:
  prev.is_none()`.)

## Proposed package layout (`ziglang-wasip2-sysroot`)

Contents (C-only; drop the bundled libc++ headers, which dominate raw size):

```
ziglang_wasip2_sysroot/
  include/wasm32-wasip2/**.h        # ~1.2 MB  (arch-independent, C-only subset)
  lib/wasm32-wasip2/
    libc.a  librt.a  crt1-command.o  crt1-reactor.o
    libclang_rt.builtins.a
    libwasi-emulated-*.a            # ~4.2 MB  (arch-independent wasm)
```

Sizes from wasi-sdk-34: the link inputs (`libc.a` + `crt1-command.o` + `builtins.a`)
are **~4.2 MB unpacked / ~1.4 MB gzipped**; the full C-only bundle is **~14 MB
unpacked, ~4 MB gzipped**. Compare to ~200 MB for the full wasi-sdk. The wasm libs
and headers are host-independent, so **one wheel serves all platforms**. The
componentizer is `wasm-tools` (single binary, published on PyPI, only needed for
wasi targets). `create-bundle.sh` produces this layout from an installed wasi-sdk.

Distribution note: the alternative -- building wasi-libc on the fly the way zig
builds musl -- is possible but heavy (needs the wasi-libc sources plus a
`wit-bindgen`-generated wasi-sockets binding step). Not worth it for wasm, which is
already portable; prefer shipping the prebuilt archives.

## Notes for SPy

- SPy emits C-only, so the libc++ headers/archives can be dropped (done above).
- If SPy needs threads, add the `wasm32-wasip2` thread libs (or use the `-threads`
  sysroot variant); not needed for the sockets demo.
- `crt1-command.o` gives a normal `main()` command component. Use `crt1-reactor.o`
  (+ reactor adapter) if SPy ever needs a library-style reactor component instead.

---

# Alternatives considered and discarded

## 1. `wasm-component-ld` instead of `wasm-tools` (works, but heavier)

The original recipe used `wasm-component-ld` to do link + componentize in one step,
driving zig's `wasm-ld` via a one-line shim (`zig-wasm-ld`):

```sh
wasm-component-ld --wasm-ld-path ./zig-wasm-ld \
    -L$SYSLIB $SYSLIB/crt1-command.o \
    app.o -lc $BUILTINS -o app.wasm
```

This is fully validated and was initially preferred because it ships its own
version-matched p1->p2 adapter (robust against `component-type` ABI drift). It is an
extra **per-host ~8 MB binary**, but that alone is not disqualifying (`wasm-tools`
is also a per-host binary of similar size).

**The decisive factor is distribution: `wasm-tools` is published on PyPI**, so it
drops straight into SPy's "everything installs via pip" story alongside `ziglang`,
with no extra packaging work. That is why it is the primary route.

However, `wasm-component-ld` remains a viable alternative: the upstream project
(https://github.com/bytecodealliance/wasm-component-ld) publishes **prebuilt binary
releases for various platforms**, so it could be adopted the same way as `ziglang`
-- i.e. repackaged into per-platform PyPI wheels. If `wasm-tools` version-matching
against the sysroot's `component-type` metadata ever becomes a problem, switching to
`wasm-component-ld` (with its bundled matching adapter) is a reasonable fallback.

## 2. Plain `zig cc` with no external sysroot (impossible)

zig 0.16 has no `wasm32-wasip2` target ("UnknownOperatingSystem") and bundles no
wasip2 socket libc. The only socket-ish thing plain `zig cc` can compile is the
wasip1 accept-subset (`zig-wasip1-accept.c`), which is useless on modern wasmtime
(the preopened-listener host path is gone). Ruled out by the hard constraints above.

## 3. A prebuilt "socket-shims" `.a` (would-have-been experiment 16)

Idea: build a tiny `sockshim.c` once with wasi-sdk that forwards to wasi-libc's
sockets, expose a BSD-ish `sockshim.h`, and have the app compile with *stock*
`zig cc --target=wasm32-wasi` against that header (no freestanding trick). It works
end-to-end, but is strictly worse than the recommended route:

- It does NOT avoid componentization, the wasip2 `libc.a`, or `wasm-tools` -- the
  supposed wins. Same dependencies, extra moving part.
- It forces a **custom, non-standard socket ABI** (`shim_*` functions), so SPy's
  emitted code would differ from native and route through hand-written wrappers. The
  recommended route lets SPy emit identical standard C for both targets.
- ABI hazards leak anyway: wasip2 constants differ from BSD (`AF_INET=1`,
  `SOCK_STREAM=6`), and `struct sockaddr` must be kept out of the shim boundary.

The freestanding-trick route gets the same result with standard C and one fewer
artifact to build and ship. Discarded.

## 4. "Plain core module with sockets" via a custom p1-on-p2 shim (out of scope)

You could extend the guest-side `wasi_snapshot_preview1` adapter, or fork wasmtime to
add socket host functions to its core-module path, to get a runnable plain core
module. Both mean either a non-standard ABI (the adapter has no p1 slot for
`socket`/`connect`) or shipping a custom runtime instead of stock `wasmtime`.
Discarded: we require stock `wasmtime`.

## 5. Own shim over `wit-bindgen` bindings (deferred, useful for WASI HTTP)

Run `wit-bindgen c` on the `wasi:sockets`/`wasi:io` WIT and write ~200-400 lines
implementing blocking connect/accept/read/write on top of the generated
Canonical-ABI glue (the async two-phase `start/finish-connect` + `subscribe`/`poll`
+ `streams.read/write` dance). This drops the dependency on wasi-libc's socket
layer but is more code and still produces a component. Not worth it *for sockets*
(wasi-libc already does this work), but it is exactly the mechanism needed later for
components with no libc wrapper, e.g. **WASI HTTP**.

---

# Open concern: `--target=wasm32-freestanding` disables libc builtins

The recommended recipe compiles with `--target=wasm32-freestanding` (to stop zig
injecting its own wasip1 headers). This has a **codegen side effect** that matters
if SPy adopts this as the *default* wasi target for ALL emitted C, not just socket
code.

## What exactly differs between `wasm32-wasi` and `wasm32-freestanding`

Measured on zig 0.16 (`zig cc -### / -dM -E / -emit-llvm`). Across the axes:

| axis | `wasm32-wasi` | `wasm32-freestanding` | addable back? |
|------|---------------|-----------------------|---------------|
| preprocessor defines | `__wasi__=1` set | `__wasi__` unset | yes: `-D__wasi__` |
| header search | +4 zig wasi-musl `-isystem` dirs | clang builtin only | yes: `-isystem` |
| link | auto crt1 + libc + `_start` + `main` rename | nothing (`_start` undefined error) | yes: link inputs + `-Dmain=__main_argc_argv` |
| LLVM `datalayout` / ABI | identical | identical | n/a (same) |
| **hosted-ness** | `__STDC_HOSTED__=1`, hosted | **`-ffreestanding`, `__STDC_HOSTED__=0`, `no-builtins`** | **NO** |
| triple OS field | `wasi0.1.0` | `unknown` | no (not a flag) |

The first three rows are exactly what the recipe re-adds. The ABI (`datalayout`,
struct-return, `long double`, varargs) is byte-identical -- verified by diffing IR.

**The load-bearing difference is hosted-ness.** zig passes `-ffreestanding` to
clang whenever the OS is `freestanding`, which sets `__STDC_HOSTED__=0` and marks
functions `no-builtins`. **This cannot be undone by any flag**: clang has
`-ffreestanding` but no `-fhosted`/`-fno-freestanding`, and `-fbuiltin` / `-O3` do
not restore it (verified: all still produce 0 idiom recognitions).

## Why the worry is justified (measured impact)

`-ffreestanding` disables the compiler's **libcall / loop-idiom recognition** (the
optimizer's TargetLibraryInfo is gated on a hosted target). Concrete measurement on
a small benchmark (`memset`/`memcpy` loops, a hand-rolled `strlen` loop, a
`printf("...\n")`):

| build | `memset`/`memcpy` loop -> intrinsic | `strlen` loop recognized | `printf` -> `puts` fold |
|-------|:---:|:---:|:---:|
| `wasm32-wasi` (hosted, zig)         | yes (2) | yes | yes |
| **our recipe** (freestanding, zig)  | **no (0)** | **no** | **no** |
| `wasm32-wasip2` (hosted, wasi-sdk clang) | yes (2) | yes | yes |

So freestanding produces **strictly worse** code than a hosted target would: a
hand-written zeroing loop stays a scalar loop instead of becoming `memset`; a length
loop is not turned into `strlen`; `printf` of a constant string is not folded to
`puts`. For the socket demo this is irrelevant, but as SPy's **default wasi target**
it would pessimize *all* emitted C -- string handling, buffer clears, struct copies,
etc. -- everywhere, not just socket code. This is a real, if usually second-order,
regression, and the concern is valid.

Note the wasi-sdk clang row: targeting real `wasm32-wasip2` (hosted) recovers full
idiom recognition. The optimization loss is a consequence of the freestanding
*workaround*, not of wasip2 itself.

## Possible mitigations (to investigate before making it the SPy default)

1. **Post-link idiom recovery is not available**; the recognition happens during
   codegen, so it must be fixed at compile time.
2. **Use wasi-sdk's clang for the compile step** (hosted `wasm32-wasip2`) and keep
   zig only for linking. Recovers builtins, but reintroduces a wasi-sdk binary
   dependency -- against the "ziglang-only compiler" goal.
3. **Get zig to expose a hosted wasip2 target.** zig 0.16 has no `wasm32-wasip2` OS
   at all; if a future zig adds it (hosted, with the right headers), the freestanding
   trick -- and this whole problem -- disappears. Worth tracking upstream.
4. **Patch/override TargetLibraryInfo via `-mllvm`** (e.g. force-enable libcalls):
   not confirmed to work here and fragile; needs investigation.
5. **Accept it** if benchmarks show the real-world SPy code impact is negligible
   (the emitted C may already call `memcpy`/`memset` explicitly rather than relying
   on idiom recognition, in which case the loss is small). Needs a representative
   SPy benchmark to decide.

Recommendation: before defaulting the SPy-to-wasi toolchain to the freestanding
recipe, benchmark option 5 on real SPy output and keep an eye on option 3 upstream.
If the regression is measurable, option 2 (wasi-sdk clang for compile, zig for link)
is the pragmatic fallback.

---

# Toolchain installed for this experiment (needed network)

- **wasmtime 48.0.1** at `/home/antocuni/wasm/wasmtime-48`, symlinked from
  `/home/antocuni/wasm/bin/wasmtime` (replaced the old 11.0.1; revert with
  `ln -sf ../wasmtime/bin/wasmtime /home/antocuni/wasm/bin/wasmtime`).
- **wasi-sdk-34** at `/home/antocuni/wasm/wasi-sdk-34` -- used here as the source of
  the wasip2 sysroot (and, in the discarded alternative, `wasm-component-ld`). A
  real deployment ships the ~14 MB C-only subset produced by `create-bundle.sh`, not
  the whole SDK.
- **wasm-tools 1.254** (standalone binary; also on PyPI) -- the componentizer.
- **ziglang 0.16** in the local `venv-zig016/` (gitignored).
