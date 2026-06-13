#include <linux/kernel.h>
#include <linux/sched/clock.h>

#include "printk_ringbuffer.h"
#include "internal.h"

/*
 * Create a second printk ringbuffer.
 * Based off of example code from kernel/printk/printk_ringbuffer.c/vprintk_store
 *
 * Logging will be turned off by the first call to print_cx_gdsc_log(), in order
 * to preserve the first detection of the issue. The full logs will be printed
 * each time print_cx_gdsc_log() is called.
 */

/*
 * First number is log2(max_entries).
 * Second number is avg string len per entry.
 */
DEFINE_PRINTKRB(test_rb, 12, 5);

static atomic_t cx_gdsc_log_disabled = ATOMIC_INIT(0);


/* Helpers from printk.c */
static inline u32 printk_caller_id(void)
{
	return in_task() ? task_pid_nr(current) :
		0x80000000 + smp_processor_id();
}

static size_t print_caller(u32 id, char *buf)
{
	char caller[12];

	snprintf(caller, sizeof(caller), "%c%u",
		 id & 0x80000000 ? 'C' : 'T', id & ~0x80000000);
	return sprintf(buf, "[%6s]", caller);
}

static size_t print_syslog(unsigned int level, char *buf)
{
	return sprintf(buf, "<%u>", level);
}

static size_t print_time(u64 ts, char *buf)
{
	unsigned long rem_nsec = do_div(ts, 1000000000);

	return sprintf(buf, "[%5lu.%06lu]",
		       (unsigned long)ts, rem_nsec / 1000);
}

static size_t info_print_prefix(const struct printk_info  *info, bool syslog,
				bool time, char *buf)
{
	size_t len = 0;

	if (syslog)
		len = print_syslog((info->facility << 3) | info->level, buf);

	if (time)
		len += print_time(info->ts_nsec, buf + len);

	len += print_caller(info->caller_id, buf + len);

	if (IS_ENABLED(CONFIG_PRINTK_CALLER) || time) {
		buf[len++] = ' ';
		buf[len] = '\0';
	}

	return len;
}

void cx_gdsc_log(const char *fmt, ...)
{
	va_list args;

	struct prb_reserved_entry e;
	struct printk_record r;
	size_t len;

	if (atomic_read(&cx_gdsc_log_disabled))
		return;

	va_start(args, fmt);
	/* Return needed buffer size, add +1 for \0 */
	len = vsnprintf(NULL, 0, fmt, args) + 1;

	// specify how much to allocate
	prb_rec_init_wr(&r, len + 1);

	if (prb_reserve(&e, &test_rb, &r)) {
		vscnprintf(r.text_buf, r.text_buf_size, fmt, args) ;

		r.info->text_len = len;
		r.info->ts_nsec = local_clock();
		r.info->caller_id = printk_caller_id();

		// commit and finalize the record
		prb_final_commit(&e);
	}

	va_end(args);
}
EXPORT_SYMBOL(cx_gdsc_log);

void print_cx_gdsc_log(void)
{
	struct printk_info info;
	struct printk_record r;
	char text_buf[1024];
	u64 seq;

	/* Disable logging */
	atomic_set(&cx_gdsc_log_disabled, 1);

	prb_rec_init_rd(&r, &info, &text_buf[0], sizeof(text_buf));

	prb_for_each_record(0, &test_rb, seq, &r) {
		char prefix[PRINTK_PREFIX_MAX];

		if (info.seq != seq)
			pr_warn("lost %llu records\n", info.seq - seq);

		if (info.text_len > r.text_buf_size) {
			pr_warn("record %llu text truncated\n", info.seq);
			text_buf[r.text_buf_size - 1] = 0;
		}

		info_print_prefix(&info, false, true, prefix);
		pr_err("%llu: %s: %s\n", info.seq, prefix,
			&text_buf[0]);
	}
}
EXPORT_SYMBOL(print_cx_gdsc_log);