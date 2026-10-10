# Refactoring notes

The starting point is `examples/before-refactor/OrderProcessor`. It compiles,
it prices the sample order at 720 cents, and it keeps every responsibility in
`process`. The maintained design under `include/` and `src/` is the result of
the refactorings below. `tests/test_regression.cpp` is the check that the
observable behavior stayed put.

## Smells in the before-image

1. **God class and long method.** `OrderProcessor::process` validates fields, interprets the plan string, writes parallel vectors, formats an event, and formats a notification.
2. **Primitive obsession.** A line is `tuple<string, int, int>`. A plan is `"standard"`, `"bulk"`, or `"premium"`. A channel is `"email"` or `"sms"`. Money is an `int` that is easy to add to a quantity.
3. **Magic numbers.** Status values `1`, `2`, `3`, and `4` are explained by a comment. The bulk rule is the literals `10` and `90`. Premium is `95`. Standard multiplies by `100` and divides by `100`.
4. **Type-code conditionals.** Plan and channel behavior are `if` / `else if` chains on strings. A new plan edits the pricing function. A new channel edits that same function.
5. **Parallel collections.** `savedIds_` and `savedTotals_` are two vectors that must stay aligned. Nothing in the type system ties an id to its total.
6. **Hidden failure.** Callers inspect an `int`. The message is a side channel, `lastError_`. Forgetting to read it looks like success when the code is `0`, and looks like an unexplained number otherwise.
7. **No seam.** Storage is a private vector. The only way to simulate a failed save or a failed send is a boolean flag on the same object (`setFailNextSave`, `setFailNextNotification`).
8. **Shotgun surgery.** Notification text, event text, and the pricing formula are local concatenations and literals. Changing the body means finding every copy inside the one method, and the method is also the validation code.

The file banner states that this is an educational anti-pattern. The defects are design defects. The function does not invoke undefined behavior, and it does not perform I/O.

## Refactorings

| Move | Before | After | Why the behavior holds |
| --- | --- | --- | --- |
| Extract Class | Validation inside `process` | `OrderValidator::validate` | Same rule order and the same messages. Regression cases call both sides with the same broken requests. |
| Replace Conditional with Polymorphism | `if (plan == "bulk")` and the percent literals | `IPricingStrategy` plus `BulkPricingStrategy` (`kMinimumQuantity = 10`, `kDiscountPercent = 10`) and `PremiumPricingStrategy` (`kDiscountPercent = 5`) | Both sides compute `subtotal * (100 - discount) / 100` only when that plan says so. Tests compare totals, including the 199-cent truncation to 189. |
| Replace Type Code with Class / Enumeration | Strings `"standard"`, `"bulk"`, `"premium"`, `"email"`, `"sms"` | `PricingPlan`, `ChannelKind` | Known values map to the same names (`OrderReceipt::strategyName` and `INotificationChannel::name`). Unknown strings in the before-image and out-of-range enumerations in the new API both fail closed with the same message. |
| Replace Magic Number with Symbolic Constant | `10`, `90`, `95` written at the use site | `BulkPricingStrategy::kMinimumQuantity`, `kDiscountPercent`, `PremiumPricingStrategy::kDiscountPercent` | The constants are the same quantities. Regression totals cover the boundary at 9 and at 10. |
| Replace Error Code with Result | Return `0`..`4` plus `lastError_` | `Result<OrderReceipt>` and `ErrorCode` | Regression maps `1` to `InvalidOrder`, `2` to `UnknownPricingPlan`, `3` to `PersistenceFailed`, and `4` to `NotificationFailed`, and compares message text. |
| Introduce Parameter Object | A flat argument list and a tuple line | `OrderRequest`, `OrderLine`, `Money` | The regression helper unpacks `OrderLine` back into the tuple the before-image still requires, then compares results. |
| Extract Method, then Replace Data Value with Object | Parallel `savedIds_` / `savedTotals_` | `StoredOrder` inside `IOrderRepository` | Success tests require the saved id and the saved total to match on both sides. |
| Extract Interface + Dependency Injection | Private vectors and channel `if`s | Constructor-injected `IOrderRepository`, `INotificationChannelFactory`, `IDomainEventPublisher`, `IPricingStrategyFactory` | Fakes record the same side effects the before-image stores in its vectors. The orchestration tests assert those records. |
| Introduce Observer | `events_.push_back(...)` inside the god class | `publisher.publish(OrderAccepted{...})` | Regression rebuilds the before-image event string with `orderAcceptedEventText` and checks the structured event fields. |

`OrderService::process` is the extracted coordinator. Its steps follow the comment stages that used to live inside `OrderProcessor::process`: validate, price, save, publish, notify.

## Why behavior should remain unchanged

A refactoring changes structure so the design can accept the next requirement, and it leaves externally visible results alone. Here the external results are:

- the total in cents for each plan, including the bulk threshold and truncating division
- the first validation message for each broken rule
- whether an id was stored
- whether an event was recorded, and the text of that record
- whether a notification was recorded, who received it, and the body
- the choice to keep the save and the event when notification fails
- the choice to skip the event and the notification when the save fails

`tests/test_regression.cpp` runs one `OrderRequest` through both implementations. The legacy path receives primitive arguments. The new path receives the request object. The assertions compare the two outcomes. Unit tests in `test_pricing.cpp`, `test_validation.cpp`, `test_events.cpp`, and `test_order_service.cpp` lock the same rules against the new types directly, so a regression failure points at a behavior change and a unit failure points at which policy moved.

Two details are design changes with the same observable failure:

- The before-image can be handed the string `"nope"` or `"pager"`. The new API uses enumerations, so those strings do not type-check. The regression test uses an out-of-range cast and still expects "pricing plan is not recognized" and "notification channel is not recognized."
- The before-image exposes `setFailNextSave` because it has no repository port. The new tests set `FakeOrderRepository::failNextSave`. Both refuse the save, skip publication, and return "failed to save order."

## Map to SOLID

| Refactoring | Principle it serves |
| --- | --- |
| Extract Class for the validator, strategies, repository, channels, and publisher | Single Responsibility: each extracted type has one reason to change |
| Replace Conditional with Polymorphism; mark concrete strategies `final` | Open/Closed: a new plan is a new `IPricingStrategy`, and `OrderService::process` stays closed |
| Substitutable `IPricingStrategy` and `INotificationChannel` with tests through the base | Liskov Substitution: callers use `quote` and `send` and do not downcast |
| Split publish from subscribe; keep `send` off the repository | Interface Segregation: `OrderService` sees `IDomainEventPublisher::publish` only |
| Constructor injection of ports; adapters implement those ports | Dependency Inversion: the use case depends on abstractions, and `src/main.cpp` supplies the details |

## What we refused to change in the name of cleanliness

The notification-failure path still saves and publishes first. Collapsing that into a single transaction would change behavior: the before-image already stored the order before a failed send. The tests forbid that "cleanup." The learning log records the outbox as future work, which is a feature change, not a refactoring.
