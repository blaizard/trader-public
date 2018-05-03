# Build the test executable
set(test_sources
	TestBase.cpp
	TestOrder.cpp
	TestPairTransactionMap.cpp
	TestTrackOrderList.cpp
	TestTransaction.cpp
)

add_executable(tradertests ${test_sources})
target_link_libraries(tradertests irstdgtest irstd trader)

enable_testing()
add_test(tests tradertests)
