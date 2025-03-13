#include "DAUniqueIDGenerater.h"
#include <atomic>
#include <chrono>
namespace DA
{

DAUniqueIDGenerater::DAUniqueIDGenerater()
{
}

uint64_t DAUniqueIDGenerater::id()
{
	static std::atomic< std::uint64_t > last_timestamp { 0 };  // 保存上一次的时间戳
	static std::atomic< std::uint32_t > sequence { 0 };        // 序列号，用于在同一纳秒内生成不同的ID

	auto now              = std::chrono::high_resolution_clock::now();
	auto nano_since_epoch = std::chrono::duration_cast< std::chrono::nanoseconds >(now.time_since_epoch()).count();

	// 如果当前时间戳与上次相同，则递增序列号；否则重置序列号
	if (nano_since_epoch == last_timestamp.load(std::memory_order_acquire)) {
		// 在同一纳秒内，递增序列号
		std::uint32_t seq = sequence.fetch_add(1, std::memory_order_relaxed);
		return ((static_cast< std::uint64_t >(nano_since_epoch) << 22) | (seq & 0x3FFFFF));
	} else {
		// 更新为新的时间戳，并将序列号设为0
		last_timestamp.store(nano_since_epoch, std::memory_order_release);
		sequence.store(0, std::memory_order_relaxed);  // 重置序列号
		return (nano_since_epoch << 22);               // 返回新的时间戳作为ID的一部分
	}
}

}
