# Educational anti-pattern

`OrderProcessor` is a deliberately poor design kept in the repository so the
refactoring notes and regression tests have a concrete starting point.

It is small, it has no undefined behavior, and it does not touch the network
or the filesystem. The defects are design defects: one type owns every
responsibility, plans and channels are strings, and failure is a bare integer.

Build the comparison demo with the root `CMakeLists.txt` target
`before_refactor_demo`. The maintained model is the `order_processing` library.
