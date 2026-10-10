# Architecture

## Layers

The maintained code is split by the direction of its dependencies.

| Layer | Types | Depends on |
| --- | --- | --- |
| Domain vocabulary | `Money`, `OrderRequest`, `OrderLine`, `StoredOrder`, `OrderReceipt`, `OrderAccepted`, `Result` | C++ standard library |
| Policies | `OrderValidator`, `IPricingStrategy` and the three plans, `LineMath` | Domain vocabulary |
| Ports | `IOrderRepository`, `IDomainEventPublisher`, `INotificationChannel`, both factories' interfaces | Domain vocabulary and `Result` |
| Application | `OrderService` | Policies and ports |
| Adapters | `InMemoryOrderRepository`, `DomainEventPublisher`, email and SMS channels, the two concrete factories | Ports |
| Composition root | `src/main.cpp`, and the harnesses in `tests/` | Application and adapters |

`OrderService.hpp` includes the port headers. It does not include adapter headers. A reader can see the dependency rule without a tool: the use case can be compiled against fakes that the production composition root has never heard of.

The before-image is outside this layering on purpose. `examples/before-refactor/OrderProcessor` is one translation unit that plays every role. The regression tests link it beside the library. Production demo code does not link it.

## Use-case flow

`OrderService::process` is the only production path that places an order.

1. `OrderValidator::validate`
2. `IPricingStrategyFactory::create` and `IPricingStrategy::quote`
3. `IOrderRepository::findById`, then `save`
4. `IDomainEventPublisher::publish`
5. `INotificationChannelFactory::create` and `INotificationChannel::send`

Each step's failure returns immediately with a `Result`. Later steps do not run, except that a failed send happens after the save and the publish have already succeeded. That exception is documented in the README tradeoffs and in `docs/learning-log.md`.

`findById` returning an empty optional means "new id." `save` returning `PersistenceFailed` means "the write was refused," including a duplicate that races past the check inside an adapter.

## Composition root

`src/main.cpp` constructs:

- `OrderValidator`
- `PricingStrategyFactory`
- `InMemoryOrderRepository`
- `DomainEventPublisher` and a printing observer
- `NotificationLog` and `NotificationChannelFactory`
- `OrderService`, passing the five collaborators

Tests build the same service with `FakeOrderRepository` and `RecordingNotificationFactory`. The service constructor is unchanged. That is the practical test of dependency inversion.

## Error policy

`Result<T>` is either a value or an `Error`. `ErrorCode` names the failed step: `InvalidOrder`, `UnknownPricingPlan`, `PersistenceFailed`, `NotificationFailed`, `UnknownChannel`. Calling `value()` or `error()` on the wrong side throws `std::logic_error`. That is a programmer mistake, not a domain failure.

The before-image returns `0` through `4`. The regression tests map those integers onto the same messages the `Result` carries.

## What the diagrams add

`docs/uml.md` draws the use case, the class relationships, the success-and-validation sequence, and the compile-time dependency direction. Those diagrams match the types in this file. If they drift, the headers are the source of truth.
