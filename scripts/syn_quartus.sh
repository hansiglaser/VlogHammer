#!/bin/bash
#
#  Vlog-Hammer -- A Verilog Synthesis Regression Test
#
#  Copyright (C) 2013  Clifford Wolf <clifford@clifford.at>
#  
#  Permission to use, copy, modify, and/or distribute this software for any
#  purpose with or without fee is hereby granted, provided that the above
#  copyright notice and this permission notice appear in all copies.
#  
#  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
#  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
#  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
#  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
#  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
#  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
#  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

if [ $# -ne 1 ]; then
	echo "Usage: $0 <job_name>" >&2
	exit 1
fi

job="$1"
set -ex --

rm -rf temp/syn_quartus_$job
mkdir -p temp/syn_quartus_$job
cd temp/syn_quartus_$job

cp ../../rtl/$job.v .
/opt/altera/13.0/quartus/bin/quartus_map $job --source=$job.v --family="Cyclone III"
/opt/altera/13.0/quartus/bin/quartus_fit $job
/opt/altera/13.0/quartus/bin/quartus_eda $job --formal_verification --tool=conformal

sed -i 's,^// DATE.*,,;' fv/conformal/$job.vo

mkdir -p ../../syn_quartus
cp fv/conformal/$job.vo ../../syn_quartus/$job.v

sync
echo READY.
