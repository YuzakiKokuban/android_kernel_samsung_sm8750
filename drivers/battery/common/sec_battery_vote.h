
#ifndef __SEC_VOTER_H
#define __SEC_VOTER_H __FILE__

#define FOREACH_VOTER(GENERATE) \
	GENERATE(VOTER_VBUS_CHANGE)   \
	GENERATE(VOTER_USB_100MA)	\
	GENERATE(VOTER_CHG_LIMIT)	\
	GENERATE(VOTER_AICL)	\
	GENERATE(VOTER_SELECT_PDO)	\
	GENERATE(VOTER_CABLE)	\
	GENERATE(VOTER_MIX_LIMIT)	\
	GENERATE(VOTER_CHG_TEMP)	\
	GENERATE(VOTER_LRP_TEMP)	\
	GENERATE(VOTER_STORE_MODE)	\
	GENERATE(VOTER_SIOP)	\
	GENERATE(VOTER_WPC_CUR)	\
	GENERATE(VOTER_SWELLING)	\
	GENERATE(VOTER_OTG)		\
	GENERATE(VOTER_SLEEP_MODE)	\
	GENERATE(VOTER_USER)	\
	GENERATE(VOTER_STEP)	\
	GENERATE(VOTER_AGING_STEP)	\
	GENERATE(VOTER_VBUS_OVP)	\
	GENERATE(VOTER_FULL_CHARGE)	\
	GENERATE(VOTER_TEST_MODE)	\
	GENERATE(VOTER_TIME_EXPIRED)	\
	GENERATE(VOTER_MUIC_ABNORMAL)	\
	GENERATE(VOTER_WC_TX)	\
	GENERATE(VOTER_SLATE)	\
	GENERATE(VOTER_SMART_SLATE)	\
	GENERATE(VOTER_SUSPEND)	\
	GENERATE(VOTER_SYSOVLO)	\
	GENERATE(VOTER_VBAT_OVP)	\
	GENERATE(VOTER_STEP_CHARGE)	\
	GENERATE(VOTER_WPC_STEP_CHARGE)	\
	GENERATE(VOTER_DC_STEP_CHARGE)	\
	GENERATE(VOTER_TOPOFF_CHANGE)	\
	GENERATE(VOTER_HMT)	\
	GENERATE(VOTER_DC_ERR)	\
	GENERATE(VOTER_DC_MODE)	\
	GENERATE(VOTER_FULL_CAPACITY)	\
	GENERATE(VOTER_WDT_EXPIRE)	\
	GENERATE(VOTER_BATTERY)	\
	GENERATE(VOTER_IFCON_WA)	\
	GENERATE(VOTER_USB_FAC_100MA)	\
	GENERATE(VOTER_PASS_THROUGH)	\
	GENERATE(VOTER_NO_BATTERY)	\
	GENERATE(VOTER_D2D_WIRE)	\
	GENERATE(VOTER_CHANGE_CHGMODE)	\
	GENERATE(VOTER_FLASH)	\
	GENERATE(VOTER_MST)	\
	GENERATE(VOTER_SRCCAP_TRANSIT)	\
	GENERATE(VOTER_FW)	\
	GENERATE(VOTER_WL_TO_W)	\
	GENERATE(VOTER_ABNORMAL_TA)	\
	GENERATE(VOTER_PHM)	\
	GENERATE(VOTER_DC_OP_MODE_F)	\
	GENERATE(VOTER_DC_OP_MODE_SYSFS)	\
	GENERATE(VOTER_DC_OP_RATIO_CHG)	\
	GENERATE(VOTER_MULTI_BATT_CHG)	\
	GENERATE(VOTER_MAX)

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

enum VOTER_ENUM {
	FOREACH_VOTER(GENERATE_ENUM)
};

enum {
	SEC_VOTE_MIN,
	SEC_VOTE_MAX,
	SEC_VOTE_EN,
};

enum {
	VOTE_PRI_0 = 0,
	VOTE_PRI_1,
	VOTE_PRI_2,
	VOTE_PRI_3,
	VOTE_PRI_4,
	VOTE_PRI_5,
	VOTE_PRI_6,
	VOTE_PRI_7,
	VOTE_PRI_8,
	VOTE_PRI_9,
	VOTE_PRI_10,
};
struct sec_vote;

extern int get_sec_vote(struct sec_vote *vote, const char **name, int *value);
extern struct sec_vote *find_vote(const char *name);
extern struct sec_vote *sec_vote_init(const char *name, int type, int num, int init_val,
		const char **voter_name, int(*cb)(void *data, int value), void *data);
extern void sec_vote_destroy(struct sec_vote *vote);
extern void _sec_vote(struct sec_vote *vote, int event, int en, int value, const char *fname, int line);
extern void sec_vote_refresh(struct sec_vote *vote);
extern const char *get_sec_keyvoter_name(struct sec_vote *vote);
extern int get_sec_vote_result(struct sec_vote *vote);
extern int get_sec_voter_status(struct sec_vote *vote, int id, int *v);
extern int show_sec_vote_status(char *buf, unsigned int p_size);
extern void change_sec_voter_pri(struct sec_vote *vote, int event, int pri);

#define sec_vote(vote, event, en, value)	_sec_vote(vote, event, en, value, __func__, __LINE__)
#define sec_votef(name, event, en, value) \
do { \
	struct sec_vote *vote = find_vote(name); \
\
	if (!vote) { \
		pr_err("%s: failed to find vote(%s)\n", __func__, (name)); \
		break; \
	} \
	_sec_vote(vote, event, en, value, __func__, __LINE__); \
} while (0)

#define sec_vote_refreshf(name) \
do { \
	struct sec_vote *vote = find_vote(name); \
\
	if (!vote) { \
		pr_err("%s: failed to find vote(%s)\n", __func__, (name)); \
		break; \
	} \
	sec_vote_refresh(vote); \
} while (0)

#define get_sec_voter_statusf(name, id, value) ({ \
int ret; \
do { \
	struct sec_vote *vote = find_vote(name); \
\
	if (!vote) { \
		pr_err("%s: failed to find vote(%s)\n", __func__, (name)); \
		ret = -EINVAL; \
		break; \
	} \
	ret = get_sec_voter_status(vote, id, value); \
} while (0); \
ret; \
})

#define get_sec_vote_resultf(name) ({ \
int ret; \
do { \
	struct sec_vote *vote = find_vote(name); \
\
	if (!vote) { \
		pr_err("%s: failed to find vote(%s)\n", __func__, (name)); \
		ret = -EINVAL; \
		break; \
	} \
	ret = get_sec_vote_result(vote); \
} while (0); \
ret; \
})
#endif
