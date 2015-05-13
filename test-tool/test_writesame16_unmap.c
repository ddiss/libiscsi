/* 
   Copyright (C) 2013 Ronnie Sahlberg <ronniesahlberg@gmail.com>
   
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

#include <CUnit/CUnit.h>

#include "iscsi.h"
#include "scsi-lowlevel.h"
#include "iscsi-support.h"
#include "iscsi-test-cu.h"


static const unsigned char zeroBlock[4096];

static int all_zeroes(const unsigned char *buf, unsigned size)
{
	unsigned j, e;

	for (j = 0; j < size; j += sizeof(zeroBlock)) {
		e = size - j;
		if (sizeof(zeroBlock) < e)
			e = sizeof(zeroBlock);
		if (memcmp(buf + j, zeroBlock, e) != 0)
			return 0;
	}

	return 1;
}

void
test_writesame16_unmap(void)
{
	int ret;
	unsigned int i;
	unsigned char *buf;

	CHECK_FOR_DATALOSS;
	CHECK_FOR_THIN_PROVISIONING;
	CHECK_FOR_LBPWS;
	CHECK_FOR_SBC;

	logging(LOG_VERBOSE, LOG_BLANK_LINE);
	logging(LOG_VERBOSE, "Test WRITESAME16 of 1-256 blocks at the start of the LUN");
	buf = calloc(65536, block_size);
	for (i = 1; i <= 256; i++) {
		logging(LOG_VERBOSE, "Write %d blocks of 0xFF", i);
		memset(buf, 0xff, i * block_size);
		ret = write16(sd, 0,
			      i * block_size, block_size, 0, 0, 0, 0, 0, buf,
			      EXPECT_STATUS_GOOD);
		logging(LOG_VERBOSE, "Unmap %d blocks using WRITESAME16", i);
		memset(buf, 0, block_size);
		ret = writesame16(sd, 0,
				  block_size, i, 0, 1, 0, 0, buf,
				  EXPECT_STATUS_GOOD);
		if (ret == -2) {
			logging(LOG_NORMAL, "[SKIPPED] WRITESAME16 is not implemented.");
			CU_PASS("[SKIPPED] Target does not support WRITESAME16. Skipping test");
			goto finished;
		}
		CU_ASSERT_EQUAL(ret, 0);

		if (rc16->lbprz) {
			logging(LOG_VERBOSE, "LBPRZ is set. Read the unmapped "
				"blocks back and verify they are all zero");
			logging(LOG_VERBOSE, "Read %d blocks and verify they "
				"are now zero", i);
			ret = read16(sd, NULL, 0,
				     i * block_size, block_size,
				     0, 0, 0, 0, 0, buf,
				     EXPECT_STATUS_GOOD);
			CU_ASSERT(all_zeroes(buf, i * block_size));
		} else {
			logging(LOG_VERBOSE, "LBPRZ is clear. Skip the read "
				"and verify zero test");
		}
	}


	logging(LOG_VERBOSE, "Test WRITESAME16 of 1-256 blocks at the end of the LUN");
	for (i = 1; i <= 256; i++) {
		logging(LOG_VERBOSE, "Write %d blocks of 0xFF", i);
		memset(buf, 0xff, i * block_size);
		ret = write16(sd, num_blocks - i,
			      i * block_size, block_size, 0, 0, 0, 0, 0, buf,
			      EXPECT_STATUS_GOOD);
		CU_ASSERT_EQUAL(ret, 0);

		logging(LOG_VERBOSE, "Unmap %d blocks using WRITESAME16", i);
		memset(buf, 0, block_size);
		ret = writesame16(sd, num_blocks - i,
				  block_size, i, 0, 1, 0, 0, buf,
				  EXPECT_STATUS_GOOD);
		CU_ASSERT_EQUAL(ret, 0);

		if (rc16->lbprz) {
			logging(LOG_VERBOSE, "LBPRZ is set. Read the unmapped "
				"blocks back and verify they are all zero");
			logging(LOG_VERBOSE, "Read %d blocks and verify they "
				"are now zero", i);
			ret = read16(sd, NULL, num_blocks - i,
				     i * block_size, block_size,
				     0, 0, 0, 0, 0, buf,
				     EXPECT_STATUS_GOOD);
			CU_ASSERT(all_zeroes(buf, i * block_size));
		} else {
			logging(LOG_VERBOSE, "LBPRZ is clear. Skip the read "
				"and verify zero test");
		}
	}

	logging(LOG_VERBOSE, "Verify that WRITESAME16 ANCHOR==1 + UNMAP==0 is invalid");
	ret = writesame16(sd, 0,
			  block_size, 1, 1, 0, 0, 0, buf,
			  EXPECT_INVALID_FIELD_IN_CDB);
	CU_ASSERT_EQUAL(ret, 0);



	if (inq_lbp->anc_sup) {
		logging(LOG_VERBOSE, "Test WRITESAME16 ANCHOR==1 + UNMAP==0");
		memset(buf, 0, block_size);
		ret = writesame16(sd, 0,
				  block_size, 1, 1, 1, 0, 0, buf,
				  EXPECT_STATUS_GOOD);
	} else {
		logging(LOG_VERBOSE, "Test WRITESAME16 ANCHOR==1 + UNMAP==0 no ANC_SUP so expecting to fail");
		ret = writesame16(sd, 0,
				  block_size, 1, 1, 1, 0, 0, buf,
				  EXPECT_INVALID_FIELD_IN_CDB);
	}

	CU_ASSERT_EQUAL(ret, 0);


	if (inq_bl == NULL) {
		logging(LOG_VERBOSE, "[FAILED] WRITESAME16 works but "
			"BlockLimits VPD is missing.");
		CU_FAIL("[FAILED] WRITESAME16 works but "
			"BlockLimits VPD is missing.");
		goto finished;
	}

	i = 256;
	if (i <= num_blocks
	    && (inq_bl->max_ws_len == 0 || inq_bl->max_ws_len >= i)) {
		logging(LOG_VERBOSE, "Block Limits VPD page reports MAX_WS_LEN "
			"as either 0 (==no limit) or >= %d. Test Unmapping "
			"%d blocks to verify that it can handle 2-byte "
			"lengths", i, i);

		logging(LOG_VERBOSE, "Write %d blocks of 0xFF", i);
		memset(buf, 0xff, i * block_size);
		ret = write16(sd, 0,
			      i * block_size, block_size, 0, 0, 0, 0, 0, buf,
			      EXPECT_STATUS_GOOD);
		CU_ASSERT_EQUAL(ret, 0);

		logging(LOG_VERBOSE, "Unmap %d blocks using WRITESAME16", i);
		memset(buf, 0, block_size);
		ret = writesame16(sd, 0,
				  block_size, i, 0, 1, 0, 0, buf,
				  EXPECT_STATUS_GOOD);
		CU_ASSERT_EQUAL(ret, 0);

		if (rc16->lbprz) {
			logging(LOG_VERBOSE, "LBPRZ is set. Read the unmapped "
				"blocks back and verify they are all zero");

			logging(LOG_VERBOSE, "Read %d blocks and verify they "
				"are now zero", i);
			ret = read16(sd, NULL, 0,
				     i * block_size, block_size,
				     0, 0, 0, 0, 0, buf,
				     EXPECT_STATUS_GOOD);
			CU_ASSERT(all_zeroes(buf, i * block_size));
		} else {
			logging(LOG_VERBOSE, "LBPRZ is clear. Skip the read "
				"and verify zero test");
		}
	} else if (i <= num_blocks) {
		logging(LOG_VERBOSE, "Block Limits VPD page reports MAX_WS_LEN "
			"as <256. Verify that a 256 block unmap fails with "
			"INVALID_FIELD_IN_CDB.");

		logging(LOG_VERBOSE, "Unmap %d blocks using WRITESAME16", i);
		ret = writesame16(sd, 0,
				  block_size, i, 0, 1, 0, 0, buf,
				  EXPECT_INVALID_FIELD_IN_CDB);
		CU_ASSERT_EQUAL(ret, 0);
	}


	i = 65536;
	if (i <= num_blocks
	    && (inq_bl->max_ws_len == 0 || inq_bl->max_ws_len >= i)) {
		logging(LOG_VERBOSE, "Block Limits VPD page reports MAX_WS_LEN "
			"as either 0 (==no limit) or >= %d. Test Unmapping "
			"%d blocks to verify that it can handle 4-byte "
			"lengths", i, i);

		logging(LOG_VERBOSE, "Write %d blocks of 0xFF", i);
		memset(buf, 0xff, i * block_size);
		ret = write16(sd, 0,
			      i * block_size, block_size, 0, 0, 0, 0, 0, buf,
			      EXPECT_STATUS_GOOD);
		CU_ASSERT_EQUAL(ret, 0);

		logging(LOG_VERBOSE, "Unmap %d blocks using WRITESAME16", i);
		memset(buf, 0, block_size);
		ret = writesame16(sd, 0,
				  block_size, i, 0, 1, 0, 0, buf,
				  EXPECT_STATUS_GOOD);
		CU_ASSERT_EQUAL(ret, 0);

		if (rc16->lbprz) {
			logging(LOG_VERBOSE, "LBPRZ is set. Read the unmapped "
				"blocks back and verify they are all zero");

			logging(LOG_VERBOSE, "Read %d blocks and verify they "
				"are now zero", i);
			ret = read16(sd, NULL, 0,
				     i * block_size, block_size,
				     0, 0, 0, 0, 0, buf,
				     EXPECT_STATUS_GOOD);
			CU_ASSERT(all_zeroes(buf, i * block_size));
		} else {
			logging(LOG_VERBOSE, "LBPRZ is clear. Skip the read "
				"and verify zero test");
		}
	} else if (i <= num_blocks) {
		logging(LOG_VERBOSE, "Block Limits VPD page reports MAX_WS_LEN "
			"as <256. Verify that a 256 block unmap fails with "
			"INVALID_FIELD_IN_CDB.");

		logging(LOG_VERBOSE, "Unmap %d blocks using WRITESAME16", i);
		ret = writesame16(sd, 0,
				  block_size, i, 0, 1, 0, 0, buf,
				  EXPECT_INVALID_FIELD_IN_CDB);
		CU_ASSERT_EQUAL(ret, 0);
	}

finished:
	free(buf);
}
