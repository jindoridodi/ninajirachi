#pragma once
#include <deque>
#include <vector>
#include <cstring>
struct Queue { size_t limit, size; std::deque<std::vector<unsigned char>> items; };
using QueueHandle_t = Queue*;
inline Queue* xQueueCreate(size_t limit, size_t size) { return new Queue{limit, size, {}}; }
inline int xQueueSend(Queue* q, const void* p, int) {
  if (q->items.size() == q->limit) return 0;
  const auto* bytes = static_cast<const unsigned char*>(p);
  q->items.emplace_back(bytes, bytes + q->size); return pdTRUE;
}
inline int xQueueReceive(Queue* q, void* p, int) {
  if (q->items.empty()) return 0;
  std::memcpy(p, q->items.front().data(), q->size); q->items.pop_front(); return pdTRUE;
}
