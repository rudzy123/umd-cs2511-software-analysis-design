# Requirements

The system places a customer order: it checks the request, prices it with one
plan, stores the priced order, publishes that the order was accepted, and
notifies the customer on a chosen channel.

## Functional requirements

1. The system accepts an order request with an id, a customer id, zero or more lines, a pricing plan, and a notification channel.
2. Each line has a SKU, a quantity, and a unit price in whole cents.
3. The system validates the request before pricing side effects. It reports the first broken rule and stops.
4. Validation rules, in order:
   - the order id is non-empty
   - the customer id is non-empty
   - there is at least one line
   - each SKU is non-empty
   - each quantity is greater than zero
   - each unit price is zero or greater
   - an email channel has an address with exactly one `@`, non-empty text on both sides, and no spaces
   - an SMS channel has a phone number of at least seven digits
   - a channel value outside the known set is rejected
5. The system prices a valid request with exactly one plan.
   - **Standard** charges the sum of quantity times unit price.
   - **Bulk** applies that sum, then 10 percent off when the total quantity is at least 10. Below 10, the sum is unchanged.
   - **Premium** applies 5 percent off the sum, at any quantity.
6. Percent discounts use integer division toward zero. A 199-cent line on the premium plan costs 189 cents.
7. Plans do not stack. An order has one plan.
8. The system refuses to store a second order with an id that is already stored.
9. After a successful save, the system publishes an order-accepted event carrying the order id, the customer id, and the total.
10. The system then sends one notification on the requested channel.
    - Email goes to the email address.
    - SMS goes to the phone number.
    - The body is `Order accepted: id=<id> customer=<id> totalCents=<total>`.
11. If validation, plan selection, or saving fails, the system does not publish and does not send.
12. If sending fails after a successful save, the save and the event remain, and the caller receives a notification failure.
13. Every failed step returns a visible error code and message. Success returns the order id, the total, the strategy name, and the channel name.

## Nonfunctional requirements

- Implementation language is C++20. The build is CMake. Unit tests use Catch2.
- Domain failures are values (`Result` / `Error`). They are not signaled by a bare status integer in the maintained design.
- The maintained library does not open network connections or files. The repository and the notification log are in-process.
- The design is single-threaded. Adapters do not lock.
- Totals are assumed to fit in a signed 32-bit integer of cents.
- The before-image under `examples/before-refactor/` stays small, defined, and clearly labeled. It exists so behavior can be compared. It is not a supported API for new callers.
- Automated tests cover pricing, validation, publication, application-service orchestration with fakes, and regression against the before-image.
- The project builds with warnings as errors on the course targets (`-Wall -Wextra -Wpedantic -Werror`).

## Use cases

### UC1 — Place order

- **Primary actor:** customer, or a sales clerk acting for the customer.
- **Preconditions:** the actor has the line items, a plan, and a channel address.
- **Main success scenario:**
  1. The actor submits an order request.
  2. The system validates the request.
  3. The system selects the pricing plan and computes the total.
  4. The system stores the priced order.
  5. The system publishes order-accepted.
  6. The system notifies the customer on the requested channel.
  7. The system returns a receipt with the total, the strategy name, and the channel name.
- **Extensions:**
  - 2a. Validation fails. The system returns the first rule message. Storage, publication, and notification do not occur.
  - 3a. The plan is not a known plan. The system returns "pricing plan is not recognized" and stops.
  - 4a. The id is already stored, or the repository cannot save. The system returns a persistence failure. Publication and notification do not occur.
  - 6a. The channel cannot send. The system returns a notification failure. The stored order and the published event remain.

### UC2 — Price an order

Included by UC1. The pricing policy depends only on the lines and the plan.

### UC3 — Notify the customer

Included by UC1. The channel depends on the request. The message body depends on the priced total.

## Assumptions

- One currency, accounted in cents. There is no tax, shipping, or currency conversion.
- The customer email and phone on the request are the notification addresses. The stored order keeps the id, customer id, total, plan, and channel.
- "Already stored" is decided inside this process. There is no external unique constraint.
- Observers run in subscription order, on the calling thread, after the save and before the notification returns.
- A real mailbox or phone gateway is out of scope. Channels record a delivery in memory so the path is observable.
- The before-image and the maintained design share the messages and totals listed above. The before-image still accepts plan and channel as strings. The maintained API uses enumerations. An out-of-range enumeration fails with the same message as an unknown string.

## Acceptance criteria

| Id | Criterion | Test |
| --- | --- | --- |
| AC1 | Standard total is the sum of line extensions | `standard pricing is the sum of line extensions` |
| AC2 | Bulk discount starts at quantity 10 and is 10 percent | `bulk pricing discounts orders that meet the quantity threshold`, `bulk pricing leaves smaller orders unchanged` |
| AC3 | Premium discount is always 5 percent | `premium pricing always applies five percent off` |
| AC4 | Percent math truncates toward zero | `discount rounding truncates toward zero` |
| AC5 | The first broken validation rule is reported | `validation reports the first broken rule` |
| AC6 | Email and SMS contact rules match the functional requirements | `contact fields are required for the selected channel` |
| AC7 | A valid order is stored, published once, and notified once | `processing a valid order prices, stores, publishes, and notifies` |
| AC8 | SMS notifications use the phone number | `sms orders notify the phone number` |
| AC9 | Invalid orders have no storage, event, or notification | `invalid orders do not leave side effects` |
| AC10 | Duplicate ids do not publish a second event | `duplicate ids are rejected after pricing and before publication` |
| AC11 | A save failure does not publish or notify | `a repository failure skips publication and notification` |
| AC12 | A send failure keeps the save and the event | `a notification failure keeps the saved order and the published event` |
| AC13 | Every subscriber receives the event, and unsubscribe removes one | `publisher notifies every subscriber`, `unsubscribe stops further delivery` |
| AC14 | The maintained design matches the before-image on totals, messages, and side effects | `tests/test_regression.cpp` |
