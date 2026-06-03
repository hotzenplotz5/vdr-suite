# Modular Make includes

This directory contains split-out Makefile fragments.

The root `Makefile` remains the public entry point for normal developer workflows:

```bash
make test
make daemon
```

Source lists and later grouped test targets can be moved here incrementally so the root `Makefile` stays maintainable.
