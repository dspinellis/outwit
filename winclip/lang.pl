#!/bin/perl
# Create key / value pair initializations from the Windows include file
print '
struct s_keyval lang[] = {
';

while (<>) {
	if (/ LANG_(\w+).*0x/) {
		$human = $id = $1;
		$human =~ s/(.)(.*)/$1 . lc($2)/e;
		print qq[\t{"$human", LANG_$id},\n];
	} elsif (/ SUBLANG_(\w+).*0x/) {
		if (!$close) {
			print '};

struct s_keyval sublang[] = {
';
			$close++;
		}
		$human = $id = $1;
		$human =~ s/(.)(.*)/$1 . lc($2)/e;
		$human =~ s/_([a-z])/'_' . uc($1)/ge;
		print qq[\t{"$human", SUBLANG_$id},\n];
	}
}
print "};\n";
