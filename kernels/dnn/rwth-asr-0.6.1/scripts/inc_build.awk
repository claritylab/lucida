BEGIN {
	FS = "="
}

{
	if ($1 == "BUILD") {
		printf "BUILD=%d\n", $2+1
	}
}

END {
}
