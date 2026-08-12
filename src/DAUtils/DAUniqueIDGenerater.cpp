#include "DAUniqueIDGenerater.h"
#include <atomic>
#include <chrono>
namespace DA
{

/**
 * @brief 构造函数
 */
DAUniqueIDGenerater::DAUniqueIDGenerater()
{
}

/**
 * @brief 生成当前进程唯一uint64_t类型的id
 *
 * 使用高分辨率时间戳和序列号组合生成唯一ID。
 * 时间戳占用高位42位，序列号占用低位22位。
 * 同一纳秒内通过递增序列号保证唯一性。
 *
 * @return 唯一的uint64_t类型id
 */
uint64_t DAUniqueIDGenerater::id_uint64()
{
	static std::atomic< std::uint64_t > last_timestamp { 0 };  // 保存上一次的时间戳
	static std::atomic< std::uint32_t > sequence { 0 };        // 序列号，用于在同一纳秒内生成不同的ID

	auto now              = std::chrono::high_resolution_clock::now();
	auto nano_since_epoch = std::chrono::duration_cast< std::chrono::nanoseconds >(now.time_since_epoch()).count();

	// 如果当前时间戳与上次相同，则递增序列号；否则通过 CAS 原子地更新时间戳并重置序列号
	uint64_t expected = last_timestamp.load(std::memory_order_acquire);
	if (static_cast< std::uint64_t >(nano_since_epoch) == expected) {
		// 在同一纳秒内，递增序列号
		std::uint32_t seq = sequence.fetch_add(1, std::memory_order_relaxed);
		return ((static_cast< std::uint64_t >(nano_since_epoch) << 22) | (seq & 0x3FFFFF));
	}
	// CAS 尝试更新时间戳，保证多变量之间的原子性
	if (last_timestamp.compare_exchange_strong(expected, static_cast< std::uint64_t >(nano_since_epoch), std::memory_order_acq_rel)) {
		// 成功更新时间戳，重置序列号
		sequence.store(0, std::memory_order_relaxed);
		return (static_cast< std::uint64_t >(nano_since_epoch) << 22);
	}
	// CAS 失败说明另一线程已更新时间戳，此时使用递增的序列号
	std::uint32_t seq = sequence.fetch_add(1, std::memory_order_relaxed);
	return ((static_cast< std::uint64_t >(nano_since_epoch) << 22) | (seq & 0x3FFFFF));
}

}
