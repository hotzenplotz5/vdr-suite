# SuiteBridge module notes

- SuiteBridge must remain independently compilable and stageable with `make -C vdr-plugin-suite-bridge`, without invoking the root daemon build targets.
- The VDR `vdr.pc` defines compile flags, `libdir` and the ABI suffix. Never hardcode a different VDR ABI into the plugin.
- Shared source headers from `../core/agent/include` are allowed; linking or running the VDR-Suite daemon to compile the plugin is not.
- Never use a real VDR installation as a test staging tree: require `DESTDIR` for staging checks.
- Add tests for changes and record behavior in `BUILDING.md`.
