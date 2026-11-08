# Test Chamber: exit-save qualification

Final source review at production baseline `12d4018c`, 2026-11-08.

`app/src/main/jni/native/test_chamber_module.cpp:792-795` attempts to save dirty
workshop state, logs a failed final write, and then unconditionally deletes the
module. Pause at lines 835-836 also attempts Save and logs failure.
`reverie_vr_workshop.h:135-151` retains dirty data in memory after failed Save and
blocks project-slot switching. These are different lifetime guarantees.

The accurate claim is: failed saves retain edits while the module remains alive
and block relevant in-module switching. Pause/Destroy make save attempts; they
do not guarantee persistence when storage remains unavailable. Home/exit can
destroy unsaved edits after rejected final writes.

This is a source-established exit-recovery gap, independently read by the final
reviewer and root. No new data-loss runtime reproduction or device test was run.
It supplements the original recent-games report's explicit final-exit limitation.

Conservative completion needs truthful unresolved-save status before ordinary
exit, a supported safe recovery/abandon path, and retention of the last durable
valid project. A module cannot promise a successful durable write while storage
remains unavailable. The matrix and consolidated report were corrected so
"changes retained" cannot be misread as surviving module destruction.
