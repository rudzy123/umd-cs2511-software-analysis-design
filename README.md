# Software Analysis and Design: Order Processing

Course case study for analyzing a small domain, designing an object-oriented
model, and protecting that design with regression tests. The domain accepts an
order, prices it, validates it, stores it, publishes an `OrderAccepted` event,
and sends a notification through a configurable channel.

The maintained model is the C++20 library under `include/` and `src/`.
`examples/before-refactor/` is a runnable anti-pattern kept so the smells,
the refactoring, and the tests have a concrete before-image. It is safe to
compile. It is a poor design to copy.

## How to read the repository

1. `docs/requirements.md` — the problem statement, use cases, and acceptance criteria.
2. `examples/before-refactor/` — one type that does every job.
3. `docs/refactoring-notes.md` — the smells and the moves that replace them.
4. `include/cs2511/` and `src/` — the design that those notes describe.
5. `tests/` — unit tests, fakes, and regression checks against the before-image.
6. `docs/solid.md`, `docs/uml.md`, and `docs/architecture.md` — principle map, diagrams, and structure.
7. `docs/learning-log.md` — decisions and the limits we accepted.

## Analysis versus design

Analysis describes the problem and the rules that any correct solution must
satisfy. For this system the analysis artifacts are the actors, the place-order
use case, the pricing rules, the validation rules, and the acceptance criteria
in `docs/requirements.md`. Analysis stays silent about C++ types.

Design assigns those responsibilities to collaborating types and chooses how
they depend on one another. The design artifacts are the class model, the
sequence of `OrderService::process`, the ports (`IOrderRepository`,
`INotificationChannel`, `IDomainEventPublisher`, `IPricingStrategy`), and the
composition root in `src/main.cpp`. A pricing rule such as "10 percent off
when the quantity reaches 10" is analysis. Putting that rule in
`BulkPricingStrategy::quote` is design.

## Architecture summary

`OrderService` is the application service for one use case. It coordinates
five collaborators, all received through its constructor:

| Step | Collaborator | Result on failure |
| --- | --- | --- |
| Validate the request | `OrderValidator` | `InvalidOrder`; nothing is stored or sent |
| Choose and run a pricing policy | `IPricingStrategyFactory` | `UnknownPricingPlan`; nothing is stored or sent |
| Reject a duplicate id, then save | `IOrderRepository` | `PersistenceFailed`; nothing is published or sent |
| Publish `OrderAccepted` | `IDomainEventPublisher` | Observers run only after a successful save |
| Open a channel and send | `INotificationChannelFactory` | `NotificationFailed`; the save and the event remain |

`src/main.cpp` is the composition root. It constructs the in-memory repository,
the event publisher, the pricing factory, and the notification factory, then
hands those objects to `OrderService`. The service header includes ports. It
does not include `InMemoryOrderRepository`, `EmailNotificationChannel`, or
`SmsNotificationChannel`.

Failures use `Result<T>` and `ErrorCode`. A missing order from `findById` is
an empty `optional`, because "not found" is a normal answer to a query. A
failed save is a `Result` error, because the caller asked the system to write.

## Cohesion and coupling

Cohesion is how focused a type's responsibilities are. `OrderValidator` only
answers whether a request is well formed. `BulkPricingStrategy` only computes a
bulk quote. `InMemoryOrderRepository` only stores priced orders. `OrderService`
only sequences the use case. Each type is easy to name with one sentence.

`examples/before-refactor/OrderProcessor` has low cohesion. Validation, the
three pricing formulas, vector storage, event text, and notification text are
one method. A change to the SMS rule edits the same function as a change to
the bulk threshold.

Coupling is how much one type must know about another. The service depends on
abstract ports, so a test can substitute `FakeOrderRepository` and
`RecordingNotificationFactory` without changing `OrderService`. The composition
root is the place that knows the concrete adapters. That dependency points
inward: adapters depend on ports, and the use case depends on ports.

The before-image couples the use case to `std::vector` storage, to string
codes such as `"bulk"` and `"email"`, and to integer return codes. There is no
seam for a fake repository.

## Composition versus inheritance

The service is composed of collaborators. Changing the pricing policy means
passing a different strategy. `OrderService` is `final` and has no protected
customization points, so a different pricing policy is a different strategy
object rather than a subclass.

Inheritance is used where a caller needs substitutable behavior:
`StandardPricingStrategy`, `BulkPricingStrategy`, and `PremiumPricingStrategy`
implement `IPricingStrategy`. Email and SMS channels implement
`INotificationChannel`. The concrete strategies are `final`, so a new policy is
a new sibling class. It is not a subclass of `StandardPricingStrategy` that
overrides a few lines and risks breaking the parent.

Shared arithmetic sits in the free functions `lineSubtotal` and `totalQuantity`.
The strategies call those functions. They do not inherit a base that prices
"almost" correctly.

## SOLID

Concrete locations are listed again, with the teaching point, in `docs/solid.md`.

**Single Responsibility.** `OrderValidator::validate` judges the request.
`IPricingStrategy::quote` prices lines. `OrderService::process` orders those
steps and does not contain the discount formula. The before-image puts all of
those reasons to change into `OrderProcessor::process`.

**Open/Closed.** A new pricing policy is a new class that implements
`IPricingStrategy`. `OrderService::process` does not grow a branch for it.
The pricing factory is the one place that learns the new plan. That hotspot is
called out under tradeoffs below.

**Liskov Substitution.** Any `IPricingStrategy` accepts a line list and returns
a `Money` quote. Tests pass `StandardPricingStrategy`, `BulkPricingStrategy`,
and `PremiumPricingStrategy` through `unique_ptr<IPricingStrategy>`. Callers
are not allowed to depend on a downcast. Preconditions stay with the validator,
so a strategy does not reject a line list that another strategy accepts.

**Interface Segregation.** `IDomainEventPublisher` exposes `publish` only.
Subscription lives on `DomainEventPublisher`, which the composition root uses.
`INotificationChannel` exposes `send`. `IOrderRepository` exposes `save` and
`findById` and has no notification operation. The before-image has one wide
surface: process, plus the vectors a caller must read to see what happened.

**Dependency Inversion.** High-level policy in `OrderService` depends on
`IPricingStrategyFactory`, `IOrderRepository`, `IDomainEventPublisher`, and
`INotificationChannelFactory`. The in-memory repository and the channel
classes depend on those same abstractions. Constructor injection is the
wiring mechanism. `src/main.cpp` and the test harnesses are the only places
that choose concrete types.

`OrderValidator` is injected as a concrete value because the case study has
one validation policy. If a second policy appeared, the next step would be a
port. Adding that port now would create an abstraction with one implementation.

## Pattern catalog

### Strategy — pricing

Intent. Define a family of pricing algorithms and make them interchangeable
behind `IPricingStrategy`.

Where. `include/cs2511/pricing/IPricingStrategy.hpp` and the three `*PricingStrategy` types. `PricingStrategyFactory` selects one from `PricingPlan`.

Benefits. The bulk threshold and the premium percent live next to the policy
they belong to. Tests call `quote` without standing up a repository. The
service treats every plan the same way: `create`, then `quote`.

Drawbacks. Each policy is a type, a file, and a factory arm. A one-line
discount is more code than a conditional. The factory must change when a plan
is added, so the strategies are open and the factory is a controlled
modification point.

### Factory — notification channels

Intent. Hide which concrete channel class to construct. The caller names a
`ChannelKind` and receives an `INotificationChannel`.

Where. `NotificationChannelFactory::create` builds `EmailNotificationChannel`
or `SmsNotificationChannel`. Both write to a `NotificationLog`. There is no
SMTP or SMS gateway in this case study.

Benefits. `OrderService` never mentions the concrete channel classes. Tests
replace the factory with `RecordingNotificationFactory`.

Drawbacks. A new channel edits the factory. Construction logic can become a
second place that knows every product. The factory also returns a `Result`,
so an out-of-range `ChannelKind` fails visibly instead of producing a null
channel.

### Observer — domain events

Intent. After an order is saved, tell interested parties without the use case
naming them.

Where. `DomainEventPublisher::subscribe`, `unsubscribe`, and `publish`.
Observers implement `IOrderAcceptedObserver::onOrderAccepted`. The service
depends only on `IDomainEventPublisher::publish`.

Benefits. A logging observer, a test recorder, or a future analytics observer
can subscribe at the composition root. `OrderService` stays unaware of them.
`publish` copies the subscriber list first, so an observer can unsubscribe
during notification without invalidating the loop.

Drawbacks. The publisher shares ownership through `shared_ptr` for the life of
the subscription. Order of notification is subscription order. A slow observer
blocks the use case, because delivery is synchronous. One event type is
modeled directly; a bus of many event types would be a later design.

### Repository — persistence

Intent. Give the use case a collection-like abstraction for priced orders so
storage technology stays outside the policy.

Where. `IOrderRepository` with `save` and `findById`. `InMemoryOrderRepository`
is the adapter. Tests use `FakeOrderRepository`, which can fail the next save.

Benefits. Duplicate detection and failure injection happen at a port. The
service policy "a failed save does not publish" is testable. Replacing the
vector with a database would be a new adapter and a composition-root change.

Drawbacks. The abstraction can hide cost. This adapter is linear and
process-local. It offers no transaction that includes the notification, which
is the partial-success tradeoff below.

## Code-smell catalog

These are visible in `examples/before-refactor/OrderProcessor`.

| Smell | Where it shows up |
| --- | --- |
| God class / long method | `OrderProcessor::process` validates, prices, stores, publishes, and notifies |
| Primitive obsession | Plan and channel are strings; each line is a `tuple<string, int, int>`; money is a bare `int` |
| Magic numbers | Return codes `1` through `4`; threshold `10`; factors `90`, `95`, and `100` |
| Switch-on-type-code | `if (plan == "bulk")` and `if (channel == "email")` |
| Parallel collections | `savedIds_` kept beside `savedTotals_` |
| Feature envy / mixed layers | Domain rules, storage, and message text edited in one function |
| Hidden failure | Meaning of the integer code lives in a header comment |
| Shotgun surgery | A new channel requires edits inside the same method that prices orders |
| Test seam as a boolean flag | `failNextSave_` and `failNextNotification_` stand in for ports |

`docs/refactoring-notes.md` maps each smell to the refactoring that replaces it.

## Refactoring workflow

1. Characterize the before-image. The tests in `tests/test_regression.cpp` run the same orders through `OrderProcessor` and `OrderService` and compare status, total, saved id, event text, and notification text.
2. Extract one responsibility at a time: validation, then pricing policy, then storage, then notification, then publication. This repository shows the result of that sequence and keeps the before-image in place.
3. Replace type-code conditionals with polymorphic strategies and a factory.
4. Replace integer codes with `Result` and `ErrorCode`.
5. Introduce `OrderLine` and `Money` so a quantity and a price are different types.
6. Rebuild and run the full suite. A refactoring is finished when the regression tests still pass and the production types each have one reason to change.

Behavior that both designs still implement is locked by those tests: totals, validation messages, duplicate rejection, save failure, and notification failure. Making an illegal plan unrepresentable in normal code is a design change. An out-of-range cast still fails closed, and the regression test checks that message.

## Build and test

Requirements: CMake 3.20 or newer, a C++20 compiler, and network access on the
first configure so CMake can fetch Catch2 3.7.1.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

Run the two demos on the same sample order (bulk, quantity 10, total 720 cents):

```bash
./build/before_refactor_demo
./build/order_demo
```

GitHub Actions workflow `.github/workflows/ci.yml` configures with Ninja and
runs the same test command.

Useful targets:

| Target | Role |
| --- | --- |
| `order_processing` | Maintained library |
| `order_demo` | Composition-root demo of the maintained design |
| `before_refactor` | Anti-pattern library |
| `before_refactor_demo` | Labeled demo of the anti-pattern |
| `cs2511_tests` | Catch2 suite |

## Design tradeoffs

- **Partial success when notification fails.** The order is saved and `OrderAccepted` is published before the channel is asked to send. A notification failure returns `NotificationFailed` and leaves those side effects in place. A blind retry hits "order id already exists." The case study chooses an explicit result over a hidden retry. A production follow-up is an outbox written in the same repository operation as the order.
- **The factories are modification hotspots.** New strategies and channels are additive types, and each factory `switch` gains an arm. A registry would move that edit to data. For three plans and two channels, a registry would hide a simple decision.
- **In-memory adapters.** The repository and the notification log live in the process. They make the use case runnable and testable. They do not survive a restart, and they do not talk to a real inbox.
- **Synchronous observers.** Publication finishes before `process` returns. That keeps the sequence diagram honest and the tests deterministic. It couples use-case latency to observer work.
- **First validation error only.** The validator returns the first broken rule, in a fixed order. Callers get one clear message. They do not get a list of every problem on the request.
- **Integer cents and truncating percent.** Discounts use integer division toward zero. `199` cents at 5 percent off is `189` cents. Amounts are assumed to fit in a signed 32-bit `int`.
- **One validation policy, injected directly.** The validator is a concrete collaborator. The varying dependencies (price, storage, events, channel) are ports.
- **Shared ownership of observers.** `subscribe` takes `shared_ptr`. That prevents a dangling observer for this teaching code. A caller that needs a precise lifetime calls `unsubscribe`.

## Assumptions that bind the design

One currency. One discount plan per order, with no stacking. Email addresses
are "one @, non-empty sides, no spaces." Phone numbers are at least seven
digits. The teaching adapters are single-threaded. Full statement:
`docs/requirements.md`.
