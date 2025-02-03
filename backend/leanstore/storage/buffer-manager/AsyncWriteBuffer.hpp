#pragma once
#include "BufferFrame.hpp"
#include "Units.hpp"
// -------------------------------------------------------------------------------------
#include "IOEngine.hpp"
// -------------------------------------------------------------------------------------
#include <functional>
#include <libxnvme.h>
// -------------------------------------------------------------------------------------
namespace leanstore
{
namespace storage
{
// -------------------------------------------------------------------------------------
class AsyncWriteBuffer
{
  private:
   struct WriteCommand {
      BufferFrame* bf;
      PID pid;
   };
   std::unique_ptr<IOEngine> io_engine;
   u64 page_size, batch_max_size;
   u64 pending_requests = 0;
   xnvme_queue *queue;

  public:
   std::unique_ptr<BufferFrame::Page[]> write_buffer;
   std::unique_ptr<WriteCommand[]> write_buffer_commands;
   // -------------------------------------------------------------------------------------
   // Debug
   // -------------------------------------------------------------------------------------
   AsyncWriteBuffer(u64 page_size, u64 batch_max_size);
   ~AsyncWriteBuffer();
   // Caller takes care of sync
   bool full();
   void add(BufferFrame& bf, PID pid);
   u64 submit();
   u64 pollEventsSync();
   void getWrittenBfs(std::function<void(BufferFrame&, u64, PID)> callback, u64 n_events);
};
// -------------------------------------------------------------------------------------
}  // namespace storage
}  // namespace leanstore
// -------------------------------------------------------------------------------------
