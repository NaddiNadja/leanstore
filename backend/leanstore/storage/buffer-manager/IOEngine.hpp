#pragma once
// -------------------------------------------------------------------------------------
#include <errno.h>
#include <libxnvme.h>
#include <iostream>
#include <memory>
#include <functional>
// -------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------

namespace leanstore
{
class LeanStore;  // Forward declaration
namespace storage
{
class IOEngine
{
  friend class IOEC;
  private:
   xnvme_dev *dev;
   uint32_t nsid;
   char *buf = NULL;
   
  public:
   // -------------------------------------------------------------------------------------
   IOEngine(const char *path);
   ~IOEngine();
   // -------------------------------------------------------------------------------------
   int initialize();
   xnvme_queue *initialiseQueue(int qdepth);

   int readAsync(void *buf, size_t nbytes, off_t offset, xnvme_queue *queue);
   int readSync(void *buf, size_t nbytes, off_t offset);
   int writeAsync(void *buf, size_t nbytes, off_t offset, xnvme_queue *queue);
   int writeSync(void *buf, size_t nbytes, off_t offset);

   int poke(int max, xnvme_queue *queue);
};
// -------------------------------------------------------------------------------------
class IOEC
{
  public:
   static IOEngine* global_io;
};
}  // namespace storage
}  // namespace leanstore
// -------------------------------------------------------------------------------------
