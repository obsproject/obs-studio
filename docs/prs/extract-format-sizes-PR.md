Title: Extract format-size helpers into modules/sources/format

Summary:
- Move small helper functions responsible for computing plane widths/heights and channel counts from `libobs/obs-source.c` into a new module `modules/sources/format`.
- Add public header `modules/sources/format/include/format_sizes.h` and implementation `src/format_sizes.c`.
- Add unit test `modules/sources/format/tests/test_format_sizes.c` and CMake rules to build/run it.
- Keep compatibility shim (extern declarations) in `libobs/obs-source.c` so behavior is unchanged.

Checklist before opening PR:
- [x] Ensure module builds with minimal features enabled (we verified with disabled optional subsystems)
- [x] Ensure `test_format_sizes` compiles and runs locally
- [ ] Add CI job to run the new unit test on Linux
- [ ] Add short note to `docs/rfc/0001-source-scene-decomposition.md` describing the extraction

Notes for reviewers:
- This is a small, behavior-preserving extraction. No public API changes other than the new module being added.
- The test validates NV12, planar 4:2:0 (I420), and RGB limited cases. Edge cases (odd widths/heights) are covered by the arithmetic in the helper functions.
- Future work: expand coverage, add static analysis and a CI test job, open a separate PR to remove the compatibility shim after full migration.
