cmake_minimum_required(VERSION 3.20)

# Lets CMake 4 configure dependencies whose own minimum is older than 3.5.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(cs2511_software_analysis_design
  VERSION 1.0.0
  DESCRIPTION "CS 2511 order-processing design case study"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)
include(FetchContent)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

FetchContent_Declare(
  Catch2
  URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.7.1.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(Catch2)

function(cs2511_apply_warnings target)
  target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endfunction()

add_library(order_processing
  src/pricing/StandardPricingStrategy.cpp
  src/pricing/BulkPricingStrategy.cpp
  src/pricing/PremiumPricingStrategy.cpp
  src/pricing/PricingStrategyFactory.cpp
  src/validation/OrderValidator.cpp
  src/persistence/InMemoryOrderRepository.cpp
  src/notifications/EmailNotificationChannel.cpp
  src/notifications/SmsNotificationChannel.cpp
  src/notifications/NotificationChannelFactory.cpp
  src/events/DomainEventPublisher.cpp
  src/application/OrderService.cpp)
target_include_directories(order_processing PUBLIC include)
cs2511_apply_warnings(order_processing)

# Educational anti-pattern. Retained so regression tests can compare behavior.
add_library(before_refactor
  examples/before-refactor/OrderProcessor.cpp)
target_include_directories(before_refactor PUBLIC examples/before-refactor)
cs2511_apply_warnings(before_refactor)

add_executable(order_demo src/main.cpp)
target_link_libraries(order_demo PRIVATE order_processing)
cs2511_apply_warnings(order_demo)

add_executable(before_refactor_demo examples/before-refactor/main.cpp)
target_link_libraries(before_refactor_demo PRIVATE before_refactor)
cs2511_apply_warnings(before_refactor_demo)

add_executable(cs2511_tests
  tests/test_pricing.cpp
  tests/test_validation.cpp
  tests/test_events.cpp
  tests/test_order_service.cpp
  tests/test_regression.cpp)
target_link_libraries(cs2511_tests PRIVATE
  order_processing
  before_refactor
  Catch2::Catch2WithMain)
target_include_directories(cs2511_tests PRIVATE tests)
cs2511_apply_warnings(cs2511_tests)

if(BUILD_TESTING)
  add_test(NAME cs2511_tests COMMAND cs2511_tests)
endif()
