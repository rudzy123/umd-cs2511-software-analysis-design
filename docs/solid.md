# SOLID in this code

Each principle below names the type that carries it and the before-image pressure that made the split worth doing.

## Single Responsibility

A type should have one reason to change.

| Type | Reason to change | Location |
| --- | --- | --- |
| `OrderValidator` | The definition of a well-formed request | `src/validation/OrderValidator.cpp`, `validate` |
| `StandardPricingStrategy`, `BulkPricingStrategy`, `PremiumPricingStrategy` | That plan's price formula | `src/pricing/` |
| `InMemoryOrderRepository` | How priced orders are retained in memory | `src/persistence/InMemoryOrderRepository.cpp` |
| `EmailNotificationChannel`, `SmsNotificationChannel` | How a message is recorded for that channel | `src/notifications/` |
| `DomainEventPublisher` | How subscribers are stored and notified | `src/events/DomainEventPublisher.cpp` |
| `OrderService` | The order of the use-case steps | `src/application/OrderService.cpp`, `process` |

`OrderProcessor::process` in `examples/before-refactor/OrderProcessor.cpp` changes when any of those reasons change. That is the cohesion problem the split removes.

## Open/Closed

A new pricing plan should be a new type, and `OrderService::process` should stay closed.

`IPricingStrategy` is the extension point (`include/cs2511/pricing/IPricingStrategy.hpp`). The three concrete strategies are `final`. Adding a plan means a new subclass of the interface, plus an arm in `PricingStrategyFactory::create`. The service still calls `create` and `quote`.

The factory is intentionally the modification hotspot. For a short, closed set of plans, that is cheaper than a plugin registry. The same shape holds for `NotificationChannelFactory::create`.

## Liskov Substitution

Code that holds `IPricingStrategy` can use any concrete strategy without inspecting the dynamic type.

`tests/test_pricing.cpp`, case `pricing strategies honor the same contract`, stores all three strategies as `unique_ptr<IPricingStrategy>` and calls `name` and `quote`. Each quote of a 1000-cent single line is between 0 and 1000 cents. Strategies do not re-validate. They honor the precondition that `OrderValidator` already enforced on the service path, and they define a total for any integer line list the unit tests pass, including an empty list (zero).

`INotificationChannel` has the same shape: `send` plus `name`. `RecordingChannel` in the tests substitutes for the email and SMS channels.

## Interface Segregation

Callers should depend on the operations they use.

| Port | Operations | Who uses it |
| --- | --- | --- |
| `IPricingStrategy` | `quote`, `name` | `OrderService` after the factory returns it |
| `IOrderRepository` | `save`, `findById` | `OrderService` |
| `INotificationChannel` | `send`, `name` | `OrderService` |
| `IDomainEventPublisher` | `publish` | `OrderService` |
| `IOrderAcceptedObserver` | `onOrderAccepted` | `DomainEventPublisher` |

`subscribe` and `unsubscribe` are on `DomainEventPublisher`, not on `IDomainEventPublisher`. The use case cannot be coupled to subscription bookkeeping. The composition root and the tests can.

`IOrderRepository` does not send mail. The before-image offers one function that does both.

## Dependency Inversion

Policy depends on abstractions. Details depend on abstractions.

`OrderService`'s constructor takes:

- `const OrderValidator&` — the one validation policy, injected so the service does not construct it
- `const IPricingStrategyFactory&`
- `IOrderRepository&`
- `IDomainEventPublisher&`
- `const INotificationChannelFactory&`

The constructor is defined in `src/application/OrderService.cpp`. The concrete adapters implement the ports they serve: `InMemoryOrderRepository` implements `IOrderRepository`, `NotificationChannelFactory` implements `INotificationChannelFactory`, `DomainEventPublisher` implements `IDomainEventPublisher`.

`src/main.cpp` is the production composition root. `ServiceHarness` in `tests/test_order_service.cpp` and `tests/test_regression.cpp` is a test composition root that supplies fakes for the repository and the channel factory while keeping the real publisher, validator, and pricing factory.

The before-image has no constructor dependencies. Its storage is a private vector. Tests that want it to fail must flip `setFailNextSave` or `setFailNextNotification`, which is a flag inside the god class rather than a substitute object.
