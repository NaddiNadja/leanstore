#include "IOEngine.hpp"
// -------------------------------------------------------------------------------------
namespace leanstore
{
namespace storage
{
IOEngine::IOEngine(const char *path)
{
    struct xnvme_opts opts = xnvme_opts_default();

    dev = xnvme_dev_open(path, &opts);
	if (!dev) {
		xnvme_cli_perr("xnvme_dev_open()", errno);
		assert(false);
	}

    nsid = xnvme_dev_get_nsid(dev);

    xnvme_dev_pr(dev, 0);
}

int IOEngine::initialize() {
    size_t buf_nbytes = xnvme_dev_get_geo(dev)->nbytes;

	buf = (char*)xnvme_buf_alloc(dev, buf_nbytes);
	if (!buf) {
		xnvme_cli_perr("xnvme_buf_alloc()", errno);
		return 1;
	}

    return 0;
}

xnvme_queue *IOEngine::initialiseQueue(int qdepth) {
    struct xnvme_queue *queue = NULL;
    int err = 0;

    err = xnvme_queue_init((xnvme_dev*)dev, qdepth, 0, &queue);
	if (err) {
		xnvme_cli_perr("xnvme_queue_init()", err);
		xnvme_dev_close((xnvme_dev*)dev);
        return NULL;
	}

    xnvme_queue_set_cb(queue, [](xnvme_cmd_ctx *ctx, void *XNVME_UNUSED(cb_arg)){
        if (xnvme_cmd_ctx_cpl_status(ctx)) {
            xnvme_cli_pinf("Command did not complete successfully");
            xnvme_cmd_ctx_pr(ctx, XNVME_PR_DEF);
        } 
        xnvme_queue_put_cmd_ctx(ctx->async.queue, ctx);
    }, NULL);

    return queue;
}

int IOEngine::readAsync(void *buf, size_t nbytes, off_t offset, xnvme_queue *queue) {
    uint64_t slba;
    uint16_t nlb;
    const xnvme_geo *geo = xnvme_dev_get_geo(dev);
    struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(queue);
    int err = 0;

    nlb = nbytes / geo->lba_nbytes - 1; // is zero-based
    slba = offset / geo->lba_nbytes;

submit:
    // Submit a read command
    err = xnvme_nvm_read(ctx, nsid, slba, nlb, buf, NULL);
    switch (err) {
        case 0:
            // xnvme_cli_pinf("Submission succeeded");
            break;

        // Submission failed: queue is full => process completions and try again
        case -EBUSY:
        case -EAGAIN:
            xnvme_queue_poke(queue, 0);
            goto submit;

        // Submission failed: unexpected error, put the command-context back in the queue
        default:
            xnvme_cli_perr("xnvme_nvm_read()", err);
            xnvme_queue_put_cmd_ctx(queue, ctx);
            return err;
    }
    return 0;
}

int IOEngine::readSync(void *buf, size_t nbytes, off_t offset) {
    uint64_t slba;
    uint16_t nlb;
    const xnvme_geo *geo = xnvme_dev_get_geo(dev);
    xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev((xnvme_dev*)dev);
    int err = 0;
    
    nlb = nbytes / geo->lba_nbytes - 1; // is zero-based
    slba = offset / geo->lba_nbytes;

    err = xnvme_nvm_read(&ctx, nsid, slba, nlb, buf, NULL);
    if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
        xnvme_cli_perr("xnvme_nvm_read()", err);
        xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
        return err ? err : -EIO;
    }
    return 0;
}

int IOEngine::writeAsync(void *buf, size_t nbytes, off_t offset, xnvme_queue *queue) {
    uint64_t slba;
    uint16_t nlb;
    const xnvme_geo *geo = xnvme_dev_get_geo(dev);
    struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(queue);
    int err = 0;

    if (!ctx) {
        return errno;
    }

    nlb = nbytes / geo->lba_nbytes - 1; // is zero-based
    slba = offset / geo->lba_nbytes;

submit:
    // Submit a write command
    err = xnvme_nvm_write(ctx, nsid, slba, nlb, buf, NULL);
    switch (err) {
        case 0:
            break;

        // Submission failed: queue is full => process completions and try again
        case -EBUSY:
        case -EAGAIN:
            xnvme_queue_poke(queue, 0);
            goto submit;

        // Submission failed: unexpected error, put the command-context back in the queue
        default:
            xnvme_cli_perr("xnvme_nvm_write()", err);
            xnvme_queue_put_cmd_ctx(queue, ctx);
            return err;
    }
    return 0;
}

int IOEngine::writeSync(void *buf, size_t nbytes, off_t offset) {
    uint64_t slba;
    uint16_t nlb;
    const xnvme_geo *geo = xnvme_dev_get_geo(dev);
    xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev((xnvme_dev*)dev);
    int err = 0;
    
    nlb = nbytes / geo->lba_nbytes - 1; // is zero-based
    slba = offset / geo->lba_nbytes;

    err = xnvme_nvm_write(&ctx, nsid, slba, nlb, buf, NULL);
    if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
        xnvme_cli_perr("xnvme_nvm_write()", err);
        xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
        return err ? err : -EIO;
    }
    return 0;
}

int IOEngine::poke(int max, xnvme_queue *queue) {
    return xnvme_queue_poke(queue, max);
}

IOEngine::~IOEngine()
{
	xnvme_buf_free(dev, buf);
    xnvme_dev_close((xnvme_dev*)dev);
}
// -------------------------------------------------------------------------------------
IOEngine* IOEC::global_io(nullptr);
}  // namespace storage
}  // namespace leanstore
// -------------------------------------------------------------------------------------
