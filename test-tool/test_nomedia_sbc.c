/* 
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

#include <CUnit/CUnit.h>

#include "iscsi.h"
#include "scsi-lowlevel.h"
#include "iscsi-support.h"
#include "iscsi-test-cu.h"

void
test_nomedia_sbc(void)
{
	int ret;
	unsigned char buf[4096];
	struct unmap_list list[1];

	CHECK_FOR_SBC;

	logging(LOG_VERBOSE, LOG_BLANK_LINE);
	logging(LOG_VERBOSE, "Test that Medium commands fail when medium is ejected on SBC devices");

	if (!inq->rmb) {
		logging(LOG_VERBOSE, "[SKIPPED] LUN is not removable. "
			"Skipping test.");
		return;
	}

	logging(LOG_VERBOSE, "Eject the medium.");
	ret = startstopunit(sd, 1, 0, 0, 0, 1, 0,
			    EXPECT_STATUS_GOOD);
	CU_ASSERT_EQUAL(ret, 0);

	logging(LOG_VERBOSE, "Test TESTUNITREADY when medium is ejected.");
	ret = testunitready(sd,
			    EXPECT_NO_MEDIUM);
	CU_ASSERT_EQUAL(ret, 0);

	logging(LOG_VERBOSE, "Test SYNCHRONIZECACHE10 when medium is ejected.");
	ret = synchronizecache10(sd, 0, 1, 1, 1,
				 EXPECT_NO_MEDIUM);
	if (ret == -2) {
		logging(LOG_NORMAL, "[SKIPPED] target does not support "
			"SYNCHRONIZECACHE10");
	} else {
		CU_ASSERT_EQUAL(ret, 0);
	}

	logging(LOG_VERBOSE, "Test SYNCHRONIZECACHE16 when medium is ejected.");
	ret = synchronizecache16(sd, 0, 1, 1, 1,
				 EXPECT_NO_MEDIUM);
	if (ret == -2) {
		logging(LOG_NORMAL, "[SKIPPED] target does not support "
			"SYNCHRONIZECACHE16");
	} else {
		CU_ASSERT_EQUAL(ret, 0);
	}

	logging(LOG_VERBOSE, "Test READ10 when medium is ejected.");
	ret = read10(sd, NULL, 0, block_size, block_size,
		     0, 0, 0, 0, 0, NULL,
		     EXPECT_NO_MEDIUM);
	CU_ASSERT_EQUAL(ret, 0);

	logging(LOG_VERBOSE, "Test READ12 when medium is ejected.");
	ret = read12(sd, NULL, 0, block_size, block_size,
		     0, 0, 0, 0, 0, NULL,
		     EXPECT_NO_MEDIUM);
	CU_ASSERT_EQUAL(ret, 0);

	logging(LOG_VERBOSE, "Test READ16 when medium is ejected.");
	ret = read16(sd, NULL, 0, block_size, block_size,
		     0, 0, 0, 0, 0, NULL,
		     EXPECT_NO_MEDIUM);
	CU_ASSERT_EQUAL(ret, 0);

	logging(LOG_VERBOSE, "Test READCAPACITY10 when medium is ejected.");
	ret = readcapacity10(sd, NULL, 0, 0,
			     EXPECT_NO_MEDIUM);
	CU_ASSERT_EQUAL(ret, 0);

	logging(LOG_VERBOSE, "Test READCAPACITY16 when medium is ejected.");
	ret = readcapacity16(sd, NULL, 15,
			     EXPECT_NO_MEDIUM);
	if (ret == -2) {
		if (sbc3_support) {
			logging(LOG_NORMAL, "[FAILED] READCAPACITY16 is not available but the device claims SBC-3 support.");
			CU_FAIL("READCAPACITY16 failed but the device claims SBC-3 support.");
		} else {
			logging(LOG_NORMAL, "[SKIPPED] READCAPACITY16 is not implemented on this target and it does not claim SBC-3 support.");
		}
	} else {
		CU_ASSERT_EQUAL(ret, 0);
	}

	logging(LOG_VERBOSE, "Test GET_LBA_STATUS when medium is ejected.");
	ret = get_lba_status(sd, NULL, 0, 24,
			     EXPECT_NO_MEDIUM);
	if (ret == -2) {
		logging(LOG_NORMAL, "[SKIPPED] target does not support "
			"GET_LBA_STATUS");
	} else {
		CU_ASSERT_EQUAL(ret, 0);
	}

	logging(LOG_VERBOSE, "Test PREFETCH10 when medium is ejected.");
	ret = prefetch10(sd, 0, 1, 1, 0, EXPECT_NO_MEDIUM);

	if (ret == -2) {
		logging(LOG_NORMAL, "[SKIPPED] target does not support "
			"PREFETCH10");
	} else {
		CU_ASSERT_EQUAL(ret, 0);
	}

	logging(LOG_VERBOSE, "Test PREFETCH16 when medium is ejected.");
	ret = prefetch16(sd, 0, 1, 1, 0, EXPECT_NO_MEDIUM);
	if (ret == -2) {
		logging(LOG_NORMAL, "[SKIPPED] target does not support "
			"PREFETCH16");
	} else {
		CU_ASSERT_EQUAL(ret, 0);
	}

	logging(LOG_VERBOSE, "Test VERIFY10 when medium is ejected.");
	memset(buf, 0xa6, sizeof(buf));
	ret = verify10(sd, 0, block_size, block_size,
		       0, 0, 1, buf,
		       EXPECT_NO_MEDIUM);
	if (ret == -2) {
		logging(LOG_NORMAL, "[SKIPPED] target does not support "
			"VERIFY10");
	} else {
		CU_ASSERT_EQUAL(ret, 0);
	}

	logging(LOG_VERBOSE, "Test VERIFY12 when medium is ejected.");
	ret = verify12(sd, 0, block_size, block_size,
		       0, 0, 1, buf,
		       EXPECT_NO_MEDIUM);
	if (ret == -2) {
		logging(LOG_NORMAL, "[SKIPPED] target does not support "
			"VERIFY102");
	} else {
		CU_ASSERT_EQUAL(ret, 0);
	}

	logging(LOG_VERBOSE, "Test VERIFY16 when medium is ejected.");
	ret = verify16(sd, 0, block_size, block_size,
		       0, 0, 1, buf,
		       EXPECT_NO_MEDIUM);
	if (ret == -2) {
		logging(LOG_NORMAL, "[SKIPPED] target does not support "
			"VERIFY16");
	} else {
		CU_ASSERT_EQUAL(ret, 0);
	}

	if (!data_loss) {
		logging(LOG_VERBOSE, "[SKIPPING] Dataloss flag not set. Skipping test for WRITE commands");
		goto finished;
	}

	logging(LOG_VERBOSE, "Test WRITE10 when medium is ejected.");
	ret = write10(sd, 0, block_size, block_size,
		      0, 0, 0, 0, 0, buf,
		      EXPECT_NO_MEDIUM);
	CU_ASSERT_EQUAL(ret, 0);

	logging(LOG_VERBOSE, "Test WRITE12 when medium is ejected.");
	ret = write12(sd, 0, block_size, block_size,
		      0, 0, 0, 0, 0, buf,
		      EXPECT_NO_MEDIUM);
	CU_ASSERT_EQUAL(ret, 0);

	logging(LOG_VERBOSE, "Test WRITE16 when medium is ejected.");
	ret = write16(sd, 0, block_size, block_size,
		      0, 0, 0, 0, 0, buf,
		      EXPECT_NO_MEDIUM);
	CU_ASSERT_EQUAL(ret, 0);

	logging(LOG_VERBOSE, "Test WRITEVERIFY10 when medium is ejected.");
	ret = writeverify10(sd, 0, block_size, block_size,
			    0, 0, 0, 0, buf,
			    EXPECT_NO_MEDIUM);
	if (ret == -2) {
		logging(LOG_NORMAL, "[SKIPPED] target does not support "
			"WRITEVERIFY10");
	} else {
		CU_ASSERT_EQUAL(ret, 0);
	}

	logging(LOG_VERBOSE, "Test WRITEVERIFY12 when medium is ejected.");
	ret = writeverify12(sd, 0, block_size, block_size,
			    0, 0, 0, 0, buf,
			    EXPECT_NO_MEDIUM);
	if (ret == -2) {
		logging(LOG_NORMAL, "[SKIPPED] target does not support "
			"WRITEVERIFY12");
	} else {
		CU_ASSERT_EQUAL(ret, 0);
	}

	logging(LOG_VERBOSE, "Test WRITEVERIFY16 when medium is ejected.");
	ret = writeverify16(sd, 0, block_size, block_size,
			    0, 0, 0, 0, buf,
			    EXPECT_NO_MEDIUM);
	if (ret == -2) {
		logging(LOG_NORMAL, "[SKIPPED] target does not support "
			"WRITEVERIFY16");
	} else {
		CU_ASSERT_EQUAL(ret, 0);
	}

	logging(LOG_VERBOSE, "Test ORWRITE when medium is ejected.");
	ret = orwrite(sd, 0, block_size, block_size,
		      0, 0, 0, 0, 0, buf,
		      EXPECT_NO_MEDIUM);
	if (ret == -2) {
		logging(LOG_NORMAL, "[SKIPPED] target does not support "
			"ORWRITE");
	} else {
		CU_ASSERT_EQUAL(ret, 0);
	}

	logging(LOG_VERBOSE, "Test COMPAREWRITE when medium is ejected.");
	logging(LOG_VERBOSE, "[SKIPPED] Test not implemented yet");

	logging(LOG_VERBOSE, "Test WRITESAME10 when medium is ejected.");
	ret = writesame10(sd, 0, block_size,
			  1, 0, 0, 0, 0, buf,
			  EXPECT_NO_MEDIUM);
	if (ret == -2) {
		logging(LOG_NORMAL, "[SKIPPED] target does not support "
			"WRITESAME10");
	} else {
		CU_ASSERT_EQUAL(ret, 0);
	}

	logging(LOG_VERBOSE, "Test WRITESAME16 when medium is ejected.");
	ret = writesame16(sd, 0, block_size,
			  1, 0, 0, 0, 0, buf,
			  EXPECT_NO_MEDIUM);
	if (ret == -2) {
		logging(LOG_NORMAL, "[SKIPPED] target does not support "
			"WRITESAME16");
	} else {
		CU_ASSERT_EQUAL(ret, 0);
	}

	logging(LOG_VERBOSE, "Test UNMAP when medium is ejected.");
	list[0].lba = 0;
	list[0].num = lbppb;
	ret = unmap(sd, 0, list, 1,
		    EXPECT_NO_MEDIUM);
	if (ret == -2) {
		logging(LOG_NORMAL, "[SKIPPED] target does not support "
			"UNMAP");
	} else {
		CU_ASSERT_EQUAL(ret, 0);
	}


finished:
	logging(LOG_VERBOSE, "Load the medium again.");
	ret = startstopunit(sd, 1, 0, 0, 0, 1, 1,
			    EXPECT_STATUS_GOOD);
	CU_ASSERT_EQUAL(ret, 0);
}
