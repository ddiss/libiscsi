/* -*-  mode:c; tab-width:8; c-basic-offset:8; indent-tabs-mode:nil;  -*- */
/*
   Copyright (C) 2018 David Disseldorp

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <arpa/inet.h>
#include <poll.h>

#include <CUnit/CUnit.h>

#include "iscsi.h"
#include "scsi-lowlevel.h"
#include "iscsi-support.h"
#include "iscsi-test-cu.h"
#include "iscsi-multipath.h"

struct test_mpio_async_prout_preempt_state {
	uint32_t dispatched;
	uint32_t completed;
};

static void
test_mpio_async_prout_preempt_cb(struct iscsi_context *iscsi __attribute__((unused)),
		       int status, void *command_data, void *private_data)
{
	struct scsi_task *atask = command_data;
	struct test_mpio_async_prout_preempt_state *state = private_data;

	state->completed++;
	/* ignore status for now */
	if (status == SCSI_STATUS_CHECK_CONDITION) {
		logging(LOG_VERBOSE, "PROUT PREEMPT failed (CmdSN=%d)",
			atask->cmdsn);
	} else {
		logging(LOG_VERBOSE, "PROUT PREEMPT succeeded (CmdSN=%d)",
			atask->cmdsn);
	}
}

static int
test_mpio_async_prout_preempt_dispatch(struct scsi_device *sdev,
              unsigned long long sark, unsigned long long rk,
              enum scsi_persistent_out_type pr_type,
	      struct test_mpio_async_prout_preempt_state *state)
{
        struct scsi_persistent_reserve_out_basic poc;
        struct scsi_task *atask;
        int ret = 0;

        memset(&poc, 0, sizeof(poc));
        poc.reservation_key = rk;
        poc.service_action_reservation_key = sark;
        atask = scsi_cdb_persistent_reserve_out(
            SCSI_PERSISTENT_RESERVE_PREEMPT,
            SCSI_PERSISTENT_RESERVE_SCOPE_LU,
            pr_type, &poc);
        CU_ASSERT_NOT_EQUAL(atask, NULL);

	ret = iscsi_scsi_command_async(sdev->iscsi_ctx,
				       sdev->iscsi_lun,
				       atask,
				       test_mpio_async_prout_preempt_cb,
				       NULL, state);
	CU_ASSERT_EQUAL(ret, 0);

	state->dispatched++;
	logging(LOG_VERBOSE, "PROUT PREEMPT dispatched (cmdsn=%d)",
		atask->cmdsn);
	return 0;
}

void
test_mpio_async_prout_preempt(void)
{
        int ret = 0;
        const unsigned long long k1 = rand_key();
        const unsigned long long k2 = rand_key();
	struct test_mpio_async_prout_preempt_state state = { 0, 0 };
        int num_uas;

        CHECK_FOR_DATALOSS;
        MPATH_SKIP_IF_UNAVAILABLE(mp_sds, mp_num_sds);
	/* only two MPIO paths used */
	CU_ASSERT(mp_num_sds >= 2);

        if (sd->iscsi_ctx == NULL) {
                const char *err = "[SKIPPED] This PERSISTENT RESERVE test is "
                        "only supported for iSCSI backends";
                logging(LOG_NORMAL, "%s", err);
                CU_PASS(err);
                return;
        }

        logging(LOG_VERBOSE, LOG_BLANK_LINE);
        logging(LOG_VERBOSE, "Test MPIO Persistent Reserve IN PREEMPT works.");

        ret = prout_register_and_ignore(mp_sds[0], k1);
        if (ret == -2) {
                CU_PASS("PERSISTENT RESERVE OUT is not implemented.");
                return;
        }
        CU_ASSERT_EQUAL(ret, 0);

        /* clear all PR state */
        ret = prout_clear(mp_sds[0], k1);
        CU_ASSERT_EQUAL(ret, 0);

        /* need to reregister cleared key */
        ret = prout_register_and_ignore(mp_sds[0], k1);
        CU_ASSERT_EQUAL(ret, 0);

        /* register secondary key */
        ret = prout_register_and_ignore(mp_sds[1], k2);
        CU_ASSERT_EQUAL(ret, 0);

	/* try to concurrently clear the second reg while... */
        ret = test_mpio_async_prout_preempt_dispatch(mp_sds[0], k1, k2,
		SCSI_PERSISTENT_RESERVE_TYPE_WRITE_EXCLUSIVE_REGISTRANTS_ONLY,
		&state);
        CU_ASSERT_EQUAL(ret, 0);

	/* ...the second reg is in use */
        ret = test_mpio_async_prout_preempt_dispatch(mp_sds[1], k2, 0,
		SCSI_PERSISTENT_RESERVE_TYPE_WRITE_EXCLUSIVE_REGISTRANTS_ONLY,
		&state);
        CU_ASSERT_EQUAL(ret, 0);

	/* wait for the responses to come in */
	while (state.completed < state.dispatched) {
		int sd_i;
		struct pollfd pfd[2];

		for (sd_i = 0; sd_i < 2; sd_i++) {
			pfd[sd_i].fd = iscsi_get_fd(mp_sds[sd_i]->iscsi_ctx);
			pfd[sd_i].events
				= iscsi_which_events(mp_sds[sd_i]->iscsi_ctx);
		}

		ret = poll(pfd, mp_num_sds, -1);
		CU_ASSERT_NOT_EQUAL(ret, -1);

		for (sd_i = 0; sd_i < 2; sd_i++) {
			if (!pfd[sd_i].revents) {
				continue;
			}
			ret = iscsi_service(mp_sds[sd_i]->iscsi_ctx,
					    pfd[sd_i].revents);
			CU_ASSERT_EQUAL(ret, 0);
		}
	}

        /* clear any UAs generated by preempt */
        ret = test_iscsi_tur_until_good(mp_sds[0], &num_uas);
        CU_ASSERT_EQUAL(ret, 0);
        ret = test_iscsi_tur_until_good(mp_sds[1], &num_uas);
        CU_ASSERT_EQUAL(ret, 0);

        /* clear all PR state */
        ret = prout_clear(mp_sds[0], k1);
        CU_ASSERT_EQUAL(ret, 0);
}
