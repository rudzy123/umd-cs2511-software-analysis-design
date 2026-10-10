# UML

The diagrams use Mermaid. Mermaid's supported diagram types cover flowcharts,
class diagrams, and sequence diagrams. A textbook use-case ellipse diagram is
drawn here as a flowchart, which is the form Mermaid can render. The system
boundary is the place-order use case and the steps it includes.

Source of truth for names: the headers under `include/cs2511/`.

## Use cases

```mermaid
flowchart LR
    Customer((Customer))
    Clerk((Sales clerk))
    Customer --> PlaceOrder[Place order]
    Clerk --> PlaceOrder
    PlaceOrder --> PriceOrder[Price the order]
    PlaceOrder --> ValidateOrder[Validate the order]
    PlaceOrder --> SaveOrder[Save the priced order]
    PlaceOrder --> PublishEvent[Publish order accepted]
    PlaceOrder --> NotifyCustomer[Notify the customer]
```

The customer and the sales clerk both start UC1, Place order. Pricing,
validation, saving, publication, and notification are included steps. They are
specified in `docs/requirements.md`.

## Class diagram

```mermaid
classDiagram
    class OrderService {
        +process(OrderRequest) Result
    }
    class OrderValidator {
        +validate(OrderRequest) Result
    }
    class IPricingStrategyFactory {
        <<interface>>
        +create(PricingPlan) Result
    }
    class PricingStrategyFactory
    class IPricingStrategy {
        <<interface>>
        +quote(lines) Money
        +name() string
    }
    class StandardPricingStrategy
    class BulkPricingStrategy
    class PremiumPricingStrategy
    class IOrderRepository {
        <<interface>>
        +save(StoredOrder) Result
        +findById(id) optional
    }
    class InMemoryOrderRepository
    class IDomainEventPublisher {
        <<interface>>
        +publish(OrderAccepted)
    }
    class DomainEventPublisher {
        +subscribe(observer)
        +unsubscribe(observer)
        +publish(OrderAccepted)
    }
    class IOrderAcceptedObserver {
        <<interface>>
        +onOrderAccepted(OrderAccepted)
    }
    class INotificationChannelFactory {
        <<interface>>
        +create(ChannelKind) Result
    }
    class NotificationChannelFactory
    class INotificationChannel {
        <<interface>>
        +send(Notification) Result
        +name() string
    }
    class EmailNotificationChannel
    class SmsNotificationChannel
    class OrderRequest
    class StoredOrder
    class OrderReceipt
    class OrderAccepted

    OrderService --> OrderValidator : validates with
    OrderService --> IPricingStrategyFactory : selects price
    OrderService --> IOrderRepository : stores
    OrderService --> IDomainEventPublisher : publishes
    OrderService --> INotificationChannelFactory : opens channel
    OrderService ..> OrderRequest : reads
    OrderService ..> OrderReceipt : returns
    OrderService ..> StoredOrder : saves
    OrderService ..> OrderAccepted : publishes
    IPricingStrategyFactory <|.. PricingStrategyFactory
    IPricingStrategyFactory ..> IPricingStrategy : creates
    IPricingStrategy <|-- StandardPricingStrategy
    IPricingStrategy <|-- BulkPricingStrategy
    IPricingStrategy <|-- PremiumPricingStrategy
    IOrderRepository <|.. InMemoryOrderRepository
    IDomainEventPublisher <|.. DomainEventPublisher
    DomainEventPublisher o--> IOrderAcceptedObserver : notifies
    INotificationChannelFactory <|.. NotificationChannelFactory
    INotificationChannelFactory ..> INotificationChannel : creates
    INotificationChannel <|-- EmailNotificationChannel
    INotificationChannel <|-- SmsNotificationChannel
```

`OrderService` depends on interfaces and on the validator. Concrete strategies,
the in-memory repository, and the two channels sit under those interfaces.
`DomainEventPublisher` both implements the publish port and holds observers.
The service never sees the observer interface.

## Sequence: process an order

```mermaid
sequenceDiagram
    participant Client
    participant Service as OrderService
    participant Validator as OrderValidator
    participant Prices as PricingStrategyFactory
    participant Strategy as IPricingStrategy
    participant Repo as IOrderRepository
    participant Events as DomainEventPublisher
    participant Channels as NotificationChannelFactory
    participant Channel as INotificationChannel

    Client->>Service: process(request)
    Service->>Validator: validate(request)
    alt request is invalid
        Validator-->>Service: InvalidOrder
        Service-->>Client: failure Result
    else request is valid
        Validator-->>Service: success
        Service->>Prices: create(plan)
        Prices-->>Service: strategy
        Service->>Strategy: quote(lines)
        Strategy-->>Service: total
        Service->>Repo: findById(orderId)
        Repo-->>Service: absent
        Service->>Repo: save(storedOrder)
        Repo-->>Service: success
        Service->>Events: publish(OrderAccepted)
        Note over Events: Observers run before publish returns
        Events-->>Service: notified
        Service->>Channels: create(channel)
        Channels-->>Service: channel
        Service->>Channel: send(notification)
        Channel-->>Service: success
        Service-->>Client: OrderReceipt
    end
```

A failed `save` returns on the `Repo` arrow and does not call `publish` or
`send`. A failed `send` returns `NotificationFailed` after `publish` has
already run. Those branches are assertions in `tests/test_order_service.cpp`.

## Dependencies

Solid arrows are "uses." Dashed arrows are "implements." The composition root
is the only production file that sees both the service and the concrete
adapters.

```mermaid
flowchart TB
    Main[src/main.cpp]
    Service[OrderService]
    Validator[OrderValidator]
    PricePort[IPricingStrategyFactory]
    RepoPort[IOrderRepository]
    EventPort[IDomainEventPublisher]
    ChannelPort[INotificationChannelFactory]
    PriceFactory[PricingStrategyFactory]
    Strategy[IPricingStrategy]
    Memory[InMemoryOrderRepository]
    Publisher[DomainEventPublisher]
    ChannelFactory[NotificationChannelFactory]
    Channel[INotificationChannel]

    Main --> Service
    Main --> Memory
    Main --> Publisher
    Main --> PriceFactory
    Main --> ChannelFactory
    Service --> Validator
    Service --> PricePort
    Service --> RepoPort
    Service --> EventPort
    Service --> ChannelPort
    PriceFactory -.-> PricePort
    PriceFactory --> Strategy
    Memory -.-> RepoPort
    Publisher -.-> EventPort
    ChannelFactory -.-> ChannelPort
    ChannelFactory --> Channel
```

`examples/before-refactor/OrderProcessor` does not appear. It does not depend
on these ports, and the ports do not depend on it. The test binary is the one
target that links both.
