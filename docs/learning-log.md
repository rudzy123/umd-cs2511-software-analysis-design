# Learning log

Date: 10 October 2026

## What we analyzed

The place-order problem is small enough to read in one sitting and large enough
to force a real split. The rules that matter are the three pricing plans, the
order of validation checks, and the side effects of a failed save versus a
failed notification. Writing those down in `docs/requirements.md` before
trusting the class diagram kept the design from inventing features. There is
no tax, no stacking of discounts, and no real mailbox.

## What was wrong with the first shape

`OrderProcessor` proved the path could run: the sample bulk order totals 720
cents, an event string is stored, and an email envelope is stored. It also
showed the maintenance cost. The bulk threshold sits in the same function as
the email check. Returning `4` means "notification failed" only if the reader
still has the header comment. Parallel vectors can drift. That file stays in
the tree, labeled, because deleting it would also delete the oracle the
regression tests compare against.

## Design decisions

- **Application service plus ports.** `OrderService` sequences the use case. Storage, pricing, events, and channels are collaborators. The service file is the one place to read when the question is "what happens, in what order?"
- **Strategy for plans that already branched.** Three pricing formulas are enough to justify polymorphism. A single formula would have stayed a function.
- **Factory at the boundary of the enum.** The service should not construct `EmailNotificationChannel`. The factory is allowed to switch on `ChannelKind` so the rest of the code does not.
- **Observer with a narrow publish port.** The use case announces `OrderAccepted`. It does not maintain the subscriber list. Tests record the event with `RecordingObserver` and also exercise `unsubscribe` on the concrete publisher.
- **Repository that can fail.** `save` returns `Result`. `findById` returns `optional`, because a missing id is not an error. The fake can fail once so the "do not publish" policy has a test.
- **`Result` instead of a status integer.** The type carries the code and the message together. The regression suite still understands the old integers, at the boundary, and checks that the messages match.
- **`Money` in cents.** Quantity and price were both `int` in the before-image. `Money` makes the unit explicit. Discount math stays integer so the truncation rule is visible.

## What the tests taught

The characterization tests in `tests/test_regression.cpp` are the safety net for the split. They failed the design whenever a message, a total, or a side effect drifted, which is what "behavior unchanged" has to mean. Fakes made the orchestration tests specific: a repository failure must leave the observer empty, and a notification failure must leave one saved order and one event. Those two cases are easy to mix up when they live in one method and easy to state when the ports are separate.

The Liskov test is deliberately boring. It asks every strategy for a name and a non-negative quote. That is the contract. The interesting differences are in the dedicated pricing cases.

## Limits we are keeping on purpose

- Notification failure is not rolled back. The result tells the caller. A second `process` of the same id fails as a duplicate. An outbox stored with the order would be the production fix, and it would be a behavior change.
- Both factories still contain a `switch`. The open/closed win is in the service and in the strategy types. Pretending the factory is open too would hide the one file that must change.
- Adapters are in-memory and single-threaded. A database repository would implement `IOrderRepository` and would be wired in `main`. It would not be a change to `OrderService`.
- Observers are synchronous and held by `shared_ptr`. That keeps the example free of dangling pointers. It is the wrong lifetime policy for a process-wide bus.
- The validator is a concrete injected object. One policy does not need an interface. The day a second policy arrives, extracting `IOrderValidator` is a small step because the service already receives the object instead of constructing it.

## How to extend the example in class

A useful exercise is to add a `SeasonalPricingStrategy` (for example, 15 percent off any order) and watch which files move: a new strategy, one factory arm, and a regression row. `OrderService.cpp` should not change. A second exercise is a `RecordingObserver` that counts events, subscribed from `main`, again without editing `process`.
