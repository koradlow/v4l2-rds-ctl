char *extract_header = "#!/usr/bin/perl\n"
	"#use strict;\n"
	"use IO::Handle;\n"
	"\n"
	"my $debug=0;\n"
	"\n"
	"sub verify ($$)\n"
	"{\n"
	"	my ($filename, $hash) = @_;\n"
	"	my ($testhash);\n"
	"\n"
	"	if (system(\"which md5sum > /dev/null 2>&1\")) {\n"
	"		die \"This firmware requires the md5sum command - see http://www.gnu.org/software/coreutils/\\n\";\n"
	"	}\n"
	"\n"
	"	open(CMD, \"md5sum \".$filename.\"|\");\n"
	"	$testhash = <CMD>;\n"
	"	$testhash =~ /([a-zA-Z0-9]*)/;\n"
	"	$testhash = $1;\n"
	"	close CMD;\n"
	"		die \"Hash of extracted file does not match (found $testhash, expected $hash!\\n\" if ($testhash ne $hash);\n"
	"}\n"
	"\n"
	"sub get_hunk ($$)\n"
	"{\n"
	"	my ($offset, $length) = @_;\n"
	"	my ($chunklength, $buf, $rcount, $out);\n"
	"\n"
	"	sysseek(INFILE, $offset, SEEK_SET);\n"
	"	while ($length > 0) {\n"
	"	# Calc chunk size\n"
	"		$chunklength = 2048;\n"
	"		$chunklength = $length if ($chunklength > $length);\n"
	"\n"
	"		$rcount = sysread(INFILE, $buf, $chunklength);\n"
	"		die \"Ran out of data\\n\" if ($rcount != $chunklength);\n"
	"		$out .= $buf;\n"
	"		$length -= $rcount;\n"
	"	}\n"
	"	return $out;\n"
	"}\n"
	"\n"
	"sub write_le16($)\n"
	"{\n"
	"	my $val = shift;\n"
	"	my $msb = ($val >> 8) &0xff;\n"
	"	my $lsb = $val & 0xff;\n"
	"\n"
	"	syswrite(OUTFILE, chr($lsb).chr($msb));\n"
	"}\n"
	"\n"
	"sub write_le32($)\n"
	"{\n"
	"	my $val = shift;\n"
	"	my $l3 = ($val >> 24) & 0xff;\n"
	"	my $l2 = ($val >> 16) & 0xff;\n"
	"	my $l1 = ($val >> 8)  & 0xff;\n"
	"	my $l0 = $val         & 0xff;\n"
	"\n"
	"	syswrite(OUTFILE, chr($l0).chr($l1).chr($l2).chr($l3));\n"
	"}\n"
	"\n"
	"sub write_le64($)\n"
	"{\n"
	"	my $val = shift;\n"
	"	my $l7 = ($val >> 56) & 0xff;\n"
	"	my $l6 = ($val >> 48) & 0xff;\n"
	"	my $l5 = ($val >> 40) & 0xff;\n"
	"	my $l4 = ($val >> 32) & 0xff;\n"
	"	my $l3 = ($val >> 24) & 0xff;\n"
	"	my $l2 = ($val >> 16) & 0xff;\n"
	"	my $l1 = ($val >> 8)  & 0xff;\n"
	"	my $l0 = $val         & 0xff;\n"
	"\n"
	"	syswrite(OUTFILE,\n"
	"		 chr($l0).chr($l1).chr($l2).chr($l3).\n"
	"		 chr($l4).chr($l5).chr($l6).chr($l7));\n"
	"}\n"
	"\n"
	"sub write_hunk($$)\n"
	"{\n"
	"	my ($offset, $length) = @_;\n"
	"	my $out = get_hunk($offset, $length);\n"
	"\n"
	"	printf \"(len %d) \",$length if ($debug);\n"
	"\n"
	"	for (my $i=0;$i<$length;$i++) {\n"
	"		printf \"%02x \",ord(substr($out,$i,1)) if ($debug);\n"
	"	}\n"
	"	printf \"\\n\" if ($debug);\n"
	"\n"
	"	syswrite(OUTFILE, $out);\n"
	"}\n"
	"\n"
	"sub write_hunk_fix_endian($$)\n"
	"{\n"
	"	my ($offset, $length) = @_;\n"
	"	my $out = get_hunk($offset, $length);\n"
	"\n"
	"	printf \"(len_fix %d) \",$length if ($debug);\n"
	"\n"
	"	for (my $i=0;$i<$length;$i++) {\n"
	"		printf \"%02x \",ord(substr($out,$i,1)) if ($debug);\n"
	"	}\n"
	"	printf \"\\n\" if ($debug);\n"
	"\n"
	"	$i=0;\n"
	"	while ($i<$length) {\n"
	"		my $size = ord(substr($out,$i,1))*256+ord(substr($out,$i+1,1));\n"
	"		syswrite(OUTFILE, substr($out,$i+1,1));\n"
	"		syswrite(OUTFILE, substr($out,$i,1));\n"
	"		$i+=2;\n"
	"		if ($size>0 && $size <0x8000) {\n"
	"			for (my $j=0;$j<$size;$j++) {\n"
	"				syswrite(OUTFILE, substr($out,$j+$i,1));\n"
	"			}\n"
	"			$i+=$size;\n"
	"		}\n"
	"	}\n"
	"}\n"
	"\n"
	"sub main_firmware($$$$)\n"
	"{\n"
	"	my $out;\n"
	"	my $j=0;\n"
	"	my $outfile = shift;\n"
	"	my $name    = shift;\n"
	"	my $version = shift;\n"
	"	my $nr_desc = shift;\n"
	"\n"
	"	for ($j = length($name); $j <32; $j++) {\n"
	"		$name = $name.chr(0);\n"
	"}\n\n"
	"	open OUTFILE, \">$outfile\";\n"
	"	syswrite(OUTFILE, $name);\n"
	"	write_le16($version);\n"
	"	write_le16($nr_desc);\n";

char *write_hunk = "\twrite_hunk(%d, %d);\n";
char *write_hunk_fix_endian = "\twrite_hunk_fix_endian(%d, %d);\n";

// Parameters: file windows filename, hash, outfile, version, name
char *end_extract = "}\n\nsub extract_firmware {\n"
	"	my $sourcefile = \"%s\";\n"
	"	my $hash = \"%s\";\n"
	"	my $outfile = \"%s\";\n"
	"	my $name = \"%s\";\n"
	"	my $version = %d;\n"
	"	my $nr_desc = %d;\n"
	"	my $out;\n"
	"\n"
	"	verify($sourcefile, $hash);\n"
	"\n"
	"	open INFILE, \"<$sourcefile\";\n"
	"	main_firmware($outfile, $name, $version, $nr_desc);\n"
	"	close INFILE;\n"
	"}\n"
	"\n"
	"extract_firmware;\n"
	"printf \"Firmwares generated.\\n\";\n";