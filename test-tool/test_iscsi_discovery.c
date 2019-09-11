/*
   Copyright (C) 2019 SUSE LLC
   Copyright (C) 2013 by Ronnie Sahlberg <ronniesahlberg@gmail.com>

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
#include <CUnit/CUnit.h>
#include <poll.h>

#include "iscsi.h"
#include "iscsi-private.h"
#include "scsi-lowlevel.h"
#include "iscsi-test-cu.h"

struct test_iscsi_discovery_state {
        int dispatched;
        int completed;
};

static void
iscsi_discovery_cb(struct iscsi_context *iscsi _U_, int status,
              void *command_data, void *private_data)
{
        struct test_iscsi_discovery_state *state = private_data;
        struct iscsi_discovery_address *da;

        CU_ASSERT_EQUAL(status, 0);
        for (da = command_data; da != NULL; da = da->next) {
		struct iscsi_target_portal *po;
		logging(LOG_VERBOSE, "Target: %s\n", da->target_name);
                for (po = da->portals; po != NULL; po = po->next) {
			logging(LOG_VERBOSE, "+ Portal: %s\n", po->portal);
                }
        }

        state->completed++;
}

void test_iscsi_discovery(void)
{
        struct test_iscsi_discovery_state state;
        int ret;

        logging(LOG_VERBOSE, LOG_BLANK_LINE);
        logging(LOG_VERBOSE, "Test SendTargets in FFP");

        if (sd->iscsi_ctx == NULL) {
                const char *err = "[SKIPPED] This test is "
                        "only supported for iSCSI backends";
                logging(LOG_NORMAL, "%s", err);
                CU_PASS(err);
                return;
        }

        memset(&state, 0, sizeof(state));
        ret = iscsi_discovery_async(sd->iscsi_ctx, iscsi_discovery_cb, &state);
        CU_ASSERT_EQUAL(ret, 0);
        state.dispatched++;

        while (state.completed < state.dispatched) {
                struct pollfd pfd;

                pfd.fd = iscsi_get_fd(sd->iscsi_ctx);
                pfd.events = iscsi_which_events(sd->iscsi_ctx);

                ret = poll(&pfd, 1, -1);
                CU_ASSERT_NOT_EQUAL(ret, -1);

                ret = iscsi_service(sd->iscsi_ctx, pfd.revents);
                CU_ASSERT_EQUAL(ret, 0);
        }
}
