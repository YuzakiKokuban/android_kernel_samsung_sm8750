struct gzip_data {
  int level;
};

struct hostname_data {
  char *F;
};

struct md5sum_data {
  int sawline;
};

struct mktemp_data {
  char *p, *tmpdir;
};

struct mount_data {
  struct arg_list *o;
  char *t, *O;

  unsigned long flags;
  char *opts;
  int okuser;
};

struct seq_data {
  char *s, *f;

  int precision, buflen;
};

struct umount_data {
  struct arg_list *t;

  char *types;
};

struct microcom_data {
  long s;

  int fd, stok;
  struct termios old_stdin, old_fd;
};

struct dos2unix_data {
  char *tempfile;
};

struct getopt_data {
  struct arg_list *l;
  char *o, *n;
};

struct nsenter_data {
  char *UupnmiC[7];
  long t;
};

struct realpath_data {
  char *R, *relative_base;
};

struct setsid_data {
  long c;
};

struct stat_data {
  char *c;

  union {
    struct stat st;
    struct statfs sf;
  } stat;
  char *file, *pattern;
  int patlen;
};

struct timeout_data {
  char *s, *k;

  struct pollfd pfd;
  sigjmp_buf sj;
  int fds[2], pid, rc;
};

struct truncate_data {
  char *s;

  long long size;
  int type;
};

struct xxd_data {
  long s, g, o, l, c;
};

struct diff_data {
  long U;
  struct arg_list *L;
  char *F, *S, *new_line_format, *old_line_format, *unchanged_line_format;

  int dir_num, size, is_binary, differ, change, len[2], *offset[2];
  struct stat st[2];
  struct {
    char **list;
    int nr_elm;
  } dir[2];
  struct {
    FILE *fp;
    int len;
  } file[2];
};

struct expr_data {
  char **tok, *delete;
};

struct tr_data {
  short *map;
  int len1, len2;
};

struct basename_data {
  char *s;
};

struct chmod_data {
  char *mode;
};

struct cmp_data {
  long n;

  int fd;
  char *name;
};

struct cp_data {
  union {
    // install's options
    struct {
      char *g, *o, *m, *t;
    } i;
    // cp's options
    struct {
      char *t, *preserve;
    } c;
  };

  char *destname;
  struct stat top;
  int (*callback)(struct dirtree *try);
  uid_t uid;
  gid_t gid;
  int pflags;
};

struct cpio_data {
  char *F, *H, *R;
};

struct cut_data {
  char *d, *O;
  struct arg_list *select[5]; // we treat them the same, so loop through

  unsigned line;
  int pairs;
  regex_t reg;
};

struct date_data {
  char *s, *r, *I, *D, *d;

  unsigned nano;
};

struct dd_data {
  // Display fields
  int show_xfer, show_records;
  unsigned long long bytes, in_full, in_part, out_full, out_part, start;
};

struct du_data {
  long d;

  unsigned long depth, total;
  dev_t st_dev;
  void *inodes;
};

struct env_data {
  struct arg_list *u;
  char *e;
};

struct file_data {
  int max_name_len;
  off_t len;
};

struct find_data {
  char **filter;
  struct double_list *argdata;
  int topdir, xdev, depth;
  time_t now;
  long max_bytes;
  char *start;
};

struct grep_data {
  long m, A, B, C;
  struct arg_list *f, *e, *M, *S, *exclude_dir;
  char *color;

  char *purple, *cyan, *red, *green, *grey;
  struct double_list *reg;
  int found, tried, delim;
  struct arg_list **fixed;
};

struct head_data {
  long c, n;

  int file_no;
};

struct id_data {
  int is_groups;
};

struct ln_data {
  char *t;
};

struct ls_data {
  long w, l, block_size;
  char *color, *sort;

  struct dirtree *files, *singledir;
  unsigned screen_width;
  int nl_title;
  char *escmore;
};

struct mkdir_data {
  char *m, *Z;
};

struct nl_data {
  char *s, *n, *b;
  long w, l, v;

  // Count of consecutive blank lines for -l has to persist between files
  long lcount, slen;
};

struct od_data {
  struct arg_list *t;
  char *A;
  long N, w, j;

  int address_idx;
  unsigned types, leftover, star;
  char *buf; // Points to buffers[0] or buffers[1].
  char *bufs[2]; // Used to detect duplicate lines.
  off_t pos;
};

struct paste_data {
  char *d;

  int files;
};

struct patch_data {
  char *i, *d;
  long v, p, g, F;

  void *current_hunk;
  long oldline, oldlen, newline, newlen, linenum, outnum;
  int context, state, filein, fileout, filepatch, hunknum;
  char *tempname;
};

struct ps_data {
  union {
    struct {
      struct arg_list *G, *g, *U, *u, *t, *s, *p, *O, *o, *P, *k;
    } ps;
    struct {
      long n, m, d, s;
      struct arg_list *u, *p, *o, *k, *O;
    } top;
    struct {
      char *L;
      struct arg_list *G, *g, *P, *s, *t, *U, *u;
      char *d;

      void *regexes, *snapshot;
      int signal;
      pid_t self, match;
    } pgrep;
  };

  struct ps_ptr_len {
    void *ptr;
    long len;
  } gg, GG, pp, PP, ss, tt, uu, UU;
  struct dirtree *threadparent;
  unsigned width, height, scroll;
  dev_t tty;
  void *fields, *kfields;
  long long ticks, bits, time;
  int kcount, forcek, sortpos, pidlen;
  int (*match_process)(long long *slot);
  void (*show_process)(void *tb);
};

struct sed_data {
  char *i;
  struct arg_list *f, *e;

  // processed pattern list
  struct double_list *pattern;

  char *nextline, *remember, *tarxform;
  void *restart, *lastregex;
  long nextlen, rememberlen, count;
  int fdout, noeol;
  unsigned xx, tarxlen, xflags;
  char delim, xftype;
};

struct sort_data {
  char *t;
  struct arg_list *k;
  char *o, *T, S;

  void *key_list;
  unsigned linecount;
  char **lines, *name;
};

struct tail_data {
  long n, c;
  char *s;

  int file_no, last_fd, ss;
  struct xnotify *not;
  struct {
    char *path;
    int fd;
    struct dev_ino di;
  } *F;
};

struct tar_data {
  char *f, *C, *I;
  struct arg_list *T, *X, *xform;
  long strip;
  char *to_command, *owner, *group, *mtime, *mode, *sort;
  struct arg_list *exclude;

  struct double_list *incl, *excl, *seen;
  struct string_list *dirs;
  char *cwd, **xfsed;
  int fd, ouid, ggid, hlc, warn, sparselen, pid, xfpipe[2];
  struct dev_ino archive_di;
  long long *sparse;
  time_t mtt;

  // hardlinks seen so far (hlc many)
  struct {
    char *arg;
    struct dev_ino di;
  } *hlx;

  // Parsed information about a tar header.
  struct tar_header {
    char *name, *link_target, *uname, *gname;
    long long size, ssize;
    uid_t uid;
    gid_t gid;
    mode_t mode;
    time_t mtime;
    dev_t device;
  } hdr;
};

struct tee_data {
  void *outputs;
  int out;
};

struct touch_data {
  char *t, *r, *d;
};

struct uniq_data {
  long w, s, f;

  long repeats;
};

struct wc_data {
  unsigned long totals[5];
};

struct xargs_data {
  long s, n, P;
  char *E;

  long entries, bytes, np;
  char delim;
  FILE *tty;
};
extern union global_union {
	struct gzip_data gzip;
	struct hostname_data hostname;
	struct md5sum_data md5sum;
	struct mktemp_data mktemp;
	struct mount_data mount;
	struct seq_data seq;
	struct umount_data umount;
	struct microcom_data microcom;
	struct dos2unix_data dos2unix;
	struct getopt_data getopt;
	struct nsenter_data nsenter;
	struct realpath_data realpath;
	struct setsid_data setsid;
	struct stat_data stat;
	struct timeout_data timeout;
	struct truncate_data truncate;
	struct xxd_data xxd;
	struct diff_data diff;
	struct expr_data expr;
	struct tr_data tr;
	struct basename_data basename;
	struct chmod_data chmod;
	struct cmp_data cmp;
	struct cp_data cp;
	struct cpio_data cpio;
	struct cut_data cut;
	struct date_data date;
	struct dd_data dd;
	struct du_data du;
	struct env_data env;
	struct file_data file;
	struct find_data find;
	struct grep_data grep;
	struct head_data head;
	struct id_data id;
	struct ln_data ln;
	struct ls_data ls;
	struct mkdir_data mkdir;
	struct nl_data nl;
	struct od_data od;
	struct paste_data paste;
	struct patch_data patch;
	struct ps_data ps;
	struct sed_data sed;
	struct sort_data sort;
	struct tail_data tail;
	struct tar_data tar;
	struct tee_data tee;
	struct touch_data touch;
	struct uniq_data uniq;
	struct wc_data wc;
	struct xargs_data xargs;
} this;
