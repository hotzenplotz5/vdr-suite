# VDR Suite Agent Plugin

This experimental plugin foundation has been superseded before merge.

VDR already exposes the native SVDRP `MOVR <id> <new name>` command and performs recording locking, in-use validation, explicit modification tracking and the final `cRecording::ChangeName()` operation itself.

VDR-Suite must therefore implement recording moves through its local Backend Agent and an SVDRP recording-action adapter instead of duplicating VDR's move operation in another plugin.

This branch remains historical draft work only and must not be installed or activated.
